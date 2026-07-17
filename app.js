"use strict";

const MORSE = {
  A: ".-", B: "-...", C: "-.-.", D: "-..", E: ".", F: "..-.", G: "--.",
  H: "....", I: "..", J: ".---", K: "-.-", L: ".-..", M: "--",
  N: "-.", O: "---", P: ".--.", Q: "--.-", R: ".-.", S: "...",
  T: "-", U: "..-", V: "...-", W: ".--", X: "-..-", Y: "-.--", Z: "--..",
  0: "-----", 1: ".----", 2: "..---", 3: "...--", 4: "....-", 5: ".....",
  6: "-....", 7: "--...", 8: "---..", 9: "----.",
  ".": ".-.-.-", ",": "--..--", "?": "..--..", "/": "-..-.", "=": "-...-",
  "+": ".-.-.", "-": "-....-", "(": "-.--.", ")": "-.--.-", ":": "---...",
  ";": "-.-.-.", "'": ".----.", "\"": ".-..-.", "@": ".--.-."
};

const DECODE = Object.fromEntries(Object.entries(MORSE).map(([k, v]) => [v, k]));
const SYMBOLS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789".split("");
const KOCH_START = "KMRSUAPTLOWI";

const state = {
  mode: "single",
  target: "",
  targetFlat: "",
  cursor: 0,
  attemptLog: [],
  currentCode: "",
  decoded: "",
  ditDown: false,
  dahDown: false,
  ditMemory: false,
  dahMemory: false,
  keyerRunning: false,
  lastWasDit: false,
  decodeTimer: 0,
  audio: null,
  oscillator: null,
  gain: null
};

const STORAGE_KEY = "paddleTrainerSettings.v1";
const LOG_STORAGE_KEY = "paddleTrainerSessionLog.v1";

const els = {
  targetPanel: document.getElementById("targetPanel"),
  targetText: document.getElementById("targetText"),
  modeLabel: document.getElementById("modeLabel"),
  wpmLabel: document.getElementById("wpmLabel"),
  inputLabel: document.getElementById("inputLabel"),
  codeLabel: document.getElementById("codeLabel"),
  resultLabel: document.getElementById("resultLabel"),
  keyStateLabel: document.getElementById("keyStateLabel"),
  logList: document.getElementById("logList"),
  logEmpty: document.getElementById("logEmpty"),
  coachState: document.getElementById("coachState"),
  coachAdvice: document.getElementById("coachAdvice"),
  coachStats: document.getElementById("coachStats"),
  coachFocus: document.getElementById("coachFocus"),
  adaptiveCoachInput: document.getElementById("adaptiveCoachInput"),
  referenceGrid: document.getElementById("referenceGrid"),
  modeSelect: document.getElementById("modeSelect"),
  wpmInput: document.getElementById("wpmInput"),
  toneInput: document.getElementById("toneInput"),
  wordLengthInput: document.getElementById("wordLengthInput"),
  wordCountInput: document.getElementById("wordCountInput"),
  iambicSelect: document.getElementById("iambicSelect"),
  symbolGrid: document.getElementById("symbolGrid")
};

function unitMs() {
  return 1200 / Number(els.wpmInput.value || 18);
}

function selectedSymbols() {
  const checked = [...els.symbolGrid.querySelectorAll("input:checked")].map((input) => input.value);
  return checked.length ? checked : ["K", "M"];
}

function randomFrom(list) {
  return list[Math.floor(Math.random() * list.length)];
}

function randomWeighted(items, weightFor) {
  const weights = items.map((item) => Math.max(1, weightFor(item)));
  const total = weights.reduce((sum, weight) => sum + weight, 0);
  let pick = Math.random() * total;
  for (let i = 0; i < items.length; i++) {
    pick -= weights[i];
    if (pick <= 0) {
      return items[i];
    }
  }
  return items[items.length - 1];
}

function makeSymbolPicker(chars) {
  if (!els.adaptiveCoachInput.checked || state.attemptLog.length < 8) {
    return () => randomFrom(chars);
  }

  const analysis = analyzeSession();
  const bySymbol = analysis.bySymbol;
  return () => randomWeighted(chars, (ch) => {
    const stat = bySymbol.get(ch);
    if (!stat) {
      return 2;
    }
    const errorRate = stat.attempts ? stat.errors / stat.attempts : 0;
    return 1 + stat.errors * 4 + Math.round(errorRate * 8);
  });
}

function randomWord(length, pickSymbol) {
  let word = "";
  for (let i = 0; i < length; i++) {
    word += pickSymbol();
  }
  return word;
}

function newTarget() {
  saveSettings();
  const chars = selectedSymbols();
  const pickSymbol = makeSymbolPicker(chars);
  const wordLength = clamp(Number(els.wordLengthInput.value || 5), 1, 12);
  const wordCount = clamp(Number(els.wordCountInput.value || 4), 2, 8);

  state.mode = els.modeSelect.value;
  state.cursor = 0;
  state.currentCode = "";
  state.decoded = "";

  if (state.mode === "single") {
    state.target = pickSymbol();
  } else if (state.mode === "word") {
    state.target = randomWord(wordLength, pickSymbol);
  } else if (state.mode === "sentence") {
    const words = [];
    for (let i = 0; i < wordCount; i++) {
      words.push(randomWord(wordLength, pickSymbol));
    }
    state.target = words.join(" ");
  } else {
    state.target = "";
  }

  state.targetFlat = state.target.replaceAll(" ", "");
  setResult(state.mode === "free" ? "Free input" : "Ready", "");
  render();
}

function render() {
  els.modeLabel.textContent = labelForMode(state.mode);
  els.wpmLabel.textContent = String(clamp(Number(els.wpmInput.value || 18), 5, 45));
  els.inputLabel.textContent = state.decoded || "-";
  els.codeLabel.textContent = state.currentCode || "-";
  els.keyStateLabel.textContent = `${state.ditDown ? "Dit" : "-"} / ${state.dahDown ? "Dah" : "-"}`;

  els.targetText.innerHTML = "";
  if (state.mode === "free") {
    const span = document.createElement("span");
    span.className = "free-output";
    span.textContent = state.decoded || "Send with paddle...";
    els.targetText.append(span);
    renderLog();
    renderCoach();
    return;
  }

  let flatIndex = 0;

  for (const ch of state.target) {
    const span = document.createElement("span");
    if (ch === " ") {
      span.className = "target-space";
      span.textContent = " ";
      els.targetText.append(span);
      continue;
    }

    span.className = "target-char";
    if (flatIndex < state.cursor) {
      span.classList.add("correct");
    }
    if (flatIndex === state.cursor) {
      span.classList.add("current");
    }
    span.textContent = ch;
    els.targetText.append(span);
    flatIndex++;
  }

  renderLog();
  renderCoach();
}

function setResult(text, className) {
  els.resultLabel.textContent = text;
  els.targetPanel.classList.remove("success", "error");
  if (className) {
    els.targetPanel.classList.add(className);
  }
}

function handleDecodedChar(ch, code) {
  state.decoded += ch;

  if (state.mode === "free") {
    addLogEntry({ actual: ch, expected: "", code, correct: null });
    setResult("Decoded", "");
    render();
    return;
  }

  const expected = state.targetFlat[state.cursor];
  const correct = ch === expected;
  addLogEntry({ actual: ch, expected, code, correct });

  if (!correct) {
    setResult(`Expected ${expected}, got ${ch}`, "error");
    if (state.mode === "single") {
      state.cursor = 0;
      state.decoded = "";
    } else {
      setTimeout(() => {
        state.cursor = 0;
        state.decoded = "";
        setResult("Try again", "");
        render();
      }, 700);
    }
    render();
    return;
  }

  state.cursor++;

  if (state.cursor >= state.targetFlat.length) {
    setResult("Correct", "success");
    render();
    setTimeout(newTarget, state.mode === "single" ? 350 : 900);
    return;
  }

  setResult("Good", "");
  render();
}

function addLogEntry(entry) {
  state.attemptLog.push(entry);
  saveSessionLog();
}

function analyzeSession() {
  const bySymbol = new Map();
  let correct = 0;

  for (const entry of state.attemptLog) {
    if (entry.correct === null || !entry.expected) {
      continue;
    }
    const expected = entry.expected || "?";
    if (!bySymbol.has(expected)) {
      bySymbol.set(expected, { symbol: expected, attempts: 0, errors: 0, correct: 0, confusions: new Map() });
    }
    const stat = bySymbol.get(expected);
    stat.attempts++;
    if (entry.correct) {
      correct++;
      stat.correct++;
    } else {
      stat.errors++;
      stat.confusions.set(entry.actual, (stat.confusions.get(entry.actual) || 0) + 1);
    }
  }

  const total = state.attemptLog.length;
  const recent = state.attemptLog.slice(-20);
  const recentCorrect = recent.filter((entry) => entry.correct).length;
  const focus = [...bySymbol.values()]
    .filter((stat) => stat.errors > 0)
    .sort((a, b) => {
      const rateA = a.errors / a.attempts;
      const rateB = b.errors / b.attempts;
      return rateB - rateA || b.errors - a.errors || a.symbol.localeCompare(b.symbol);
    })
    .slice(0, 6);

  return {
    total,
    correct,
    accuracy: total ? correct / total : 0,
    recentTotal: recent.length,
    recentAccuracy: recent.length ? recentCorrect / recent.length : 0,
    bySymbol,
    focus
  };
}

function renderCoach() {
  const analysis = analyzeSession();
  const pct = Math.round(analysis.accuracy * 100);
  const recentPct = Math.round(analysis.recentAccuracy * 100);
  const focusSymbols = analysis.focus.map((stat) => stat.symbol);

  els.coachState.textContent = els.adaptiveCoachInput.checked ? "Adaptive" : "Watching";
  els.coachAdvice.textContent = coachAdviceText(analysis);

  els.coachStats.innerHTML = "";
  const statItems = [
    ["Total", analysis.total],
    ["Accuracy", analysis.total ? `${pct}%` : "-"],
    ["Recent", analysis.recentTotal ? `${recentPct}%` : "-"],
    ["WPM", clamp(Number(els.wpmInput.value || 18), 5, 45)]
  ];

  for (const [label, value] of statItems) {
    const item = document.createElement("div");
    item.className = "coach-stat";
    const small = document.createElement("span");
    small.textContent = label;
    const strong = document.createElement("strong");
    strong.textContent = value;
    item.append(small, strong);
    els.coachStats.append(item);
  }

  els.coachFocus.innerHTML = "";
  if (!focusSymbols.length) {
    const chip = document.createElement("span");
    chip.className = "focus-chip";
    chip.textContent = "collecting";
    els.coachFocus.append(chip);
  } else {
    for (const stat of analysis.focus) {
      const chip = document.createElement("span");
      chip.className = `focus-chip ${stat.errors ? "hot" : ""}`;
      chip.textContent = `${stat.symbol} ${stat.errors}/${stat.attempts}`;
      els.coachFocus.append(chip);
    }
  }
}

function coachAdviceText(analysis) {
  if (analysis.total < 10) {
    return "Send 10-20 symbols. I will start favoring symbols where errors appear.";
  }

  const focusSymbols = analysis.focus.map((stat) => stat.symbol);
  if (analysis.recentTotal >= 10 && analysis.recentAccuracy < 0.7) {
    return `Too many recent errors. Lower WPM by 2-3 and drill ${focusSymbols.join(", ") || "the current set"}.`;
  }

  if (focusSymbols.length) {
    return `Current focus: ${focusSymbols.join(", ")}. Adaptive mode will show them more often.`;
  }

  if (analysis.total >= 30 && analysis.accuracy >= 0.9) {
    return "Accuracy is high. Increase WPM by 1-2 or add new symbols.";
  }

  return "Pace looks reasonable. Continue to 30-50 symbols, then expand the set.";
}

function renderLog() {
  els.logList.innerHTML = "";

  if (!state.attemptLog.length) {
    const empty = document.createElement("span");
    empty.id = "logEmpty";
    empty.className = "log-empty";
    empty.textContent = "No input yet";
    els.logList.append(empty);
    return;
  }

  for (const entry of state.attemptLog) {
    const item = document.createElement("span");
    item.className = `log-entry ${entry.correct === null ? "neutral" : entry.correct ? "correct" : "error"}`;

    const symbols = document.createElement("span");
    symbols.className = "log-symbols";

    const actual = document.createElement("span");
    actual.className = "log-actual";
    actual.textContent = entry.actual;

    const expected = document.createElement("span");
    expected.className = "log-expected";
    expected.textContent = entry.expected ? `/${entry.expected}` : "";

    const code = document.createElement("span");
    code.className = "log-code";
    code.textContent = entry.code;

    symbols.append(actual, expected);
    item.append(symbols, code);
    els.logList.append(item);
  }
}

function startAudio() {
  if (!state.audio) {
    state.audio = new AudioContext();
    state.gain = state.audio.createGain();
    state.gain.gain.value = 0.16;
    state.gain.connect(state.audio.destination);
  }
  if (state.audio.state === "suspended") {
    state.audio.resume();
  }
}

function toneOn() {
  startAudio();
  if (state.oscillator) {
    return;
  }
  const osc = state.audio.createOscillator();
  osc.type = "sine";
  osc.frequency.value = Number(els.toneInput.value || 700);
  osc.connect(state.gain);
  osc.start();
  state.oscillator = osc;
}

function toneOff() {
  if (!state.oscillator) {
    return;
  }
  state.oscillator.stop();
  state.oscillator.disconnect();
  state.oscillator = null;
}

function sendElement(mark) {
  clearTimeout(state.decodeTimer);
  const duration = mark === "." ? unitMs() : unitMs() * 3;
  state.currentCode += mark;
  render();
  toneOn();
  return sleep(duration).then(() => {
    toneOff();
    scheduleDecode();
  });
}

function scheduleDecode() {
  clearTimeout(state.decodeTimer);
  state.decodeTimer = setTimeout(() => {
    if (!state.currentCode) {
      return;
    }
    const code = state.currentCode;
    const decoded = DECODE[code] || "#";
    state.currentCode = "";
    handleDecodedChar(decoded, code);
  }, unitMs() * 3);
}

async function runKeyer() {
  if (state.keyerRunning) {
    return;
  }

  state.keyerRunning = true;
  clearTimeout(state.decodeTimer);

  while (state.ditDown || state.dahDown || state.ditMemory || state.dahMemory) {
    if (state.ditDown) {
      state.ditMemory = true;
    }
    if (state.dahDown) {
      state.dahMemory = true;
    }

    let sendDit;
    if (state.ditMemory && state.dahMemory) {
      sendDit = !state.lastWasDit;
    } else {
      sendDit = state.ditMemory;
    }

    if (sendDit) {
      state.ditMemory = false;
      await sendElement(".");
    } else {
      state.dahMemory = false;
      await sendElement("-");
    }

    state.lastWasDit = sendDit;
    await sleep(unitMs());

    if (els.iambicSelect.value === "a") {
      if (!state.ditDown) {
        state.ditMemory = false;
      }
      if (!state.dahDown) {
        state.dahMemory = false;
      }
    }
  }

  state.keyerRunning = false;
  scheduleDecode();
  render();
}

function onKeyDown(event) {
  if (event.repeat) {
    return;
  }
  if (event.key === "[") {
    event.preventDefault();
    state.ditDown = true;
    state.ditMemory = true;
    setResult("Sending", "");
    runKeyer();
    render();
  } else if (event.key === "]") {
    event.preventDefault();
    state.dahDown = true;
    state.dahMemory = true;
    setResult("Sending", "");
    runKeyer();
    render();
  }
}

function onKeyUp(event) {
  if (event.key === "[") {
    event.preventDefault();
    state.ditDown = false;
    render();
  } else if (event.key === "]") {
    event.preventDefault();
    state.dahDown = false;
    render();
  }
}

function buildSymbolGrid() {
  els.symbolGrid.innerHTML = "";
  const settings = loadSettings();
  const selected = settings?.symbols?.length ? settings.symbols : KOCH_START.split("");

  for (const symbol of SYMBOLS) {
    const label = document.createElement("label");
    label.className = "symbol-tile";
    const input = document.createElement("input");
    input.type = "checkbox";
    input.value = symbol;
    input.checked = selected.includes(symbol);
    input.addEventListener("change", () => {
      label.classList.toggle("selected", input.checked);
      newTarget();
    });
    label.append(input, document.createTextNode(symbol));
    label.classList.toggle("selected", input.checked);
    els.symbolGrid.append(label);
  }
}

function buildReferenceGrid() {
  const order = [
    ..."ABCDEFGHIJKLMNOPQRSTUVWXYZ",
    ..."0123456789",
    ".", ",", "?", "/", "=", "+", "-", "(", ")", ":", ";", "'", "\"", "@"
  ];

  els.referenceGrid.innerHTML = "";
  for (const ch of order) {
    const item = document.createElement("div");
    item.className = "reference-item";

    const char = document.createElement("span");
    char.className = "reference-char";
    char.textContent = ch;

    const code = document.createElement("span");
    code.className = "reference-code";
    code.textContent = MORSE[ch];

    item.append(char, code);
    els.referenceGrid.append(item);
  }
}

function loadSettings() {
  try {
    return JSON.parse(localStorage.getItem(STORAGE_KEY) || "null");
  } catch {
    return null;
  }
}

function loadSessionLog() {
  try {
    const log = JSON.parse(sessionStorage.getItem(LOG_STORAGE_KEY) || "[]");
    return Array.isArray(log) ? log : [];
  } catch {
    return [];
  }
}

function saveSessionLog() {
  sessionStorage.setItem(LOG_STORAGE_KEY, JSON.stringify(state.attemptLog));
}

function applySettings() {
  const settings = loadSettings();
  if (!settings) {
    return;
  }

  if (settings.mode) {
    els.modeSelect.value = settings.mode;
  }
  if (settings.wpm) {
    els.wpmInput.value = settings.wpm;
  }
  if (settings.tone) {
    els.toneInput.value = settings.tone;
  }
  if (settings.wordLength) {
    els.wordLengthInput.value = settings.wordLength;
  }
  if (settings.wordCount) {
    els.wordCountInput.value = settings.wordCount;
  }
  if (settings.iambic) {
    els.iambicSelect.value = settings.iambic;
  }
  if (typeof settings.adaptiveCoach === "boolean") {
    els.adaptiveCoachInput.checked = settings.adaptiveCoach;
  }
}

function saveSettings() {
  const settings = {
    mode: els.modeSelect.value,
    wpm: els.wpmInput.value,
    tone: els.toneInput.value,
    wordLength: els.wordLengthInput.value,
    wordCount: els.wordCountInput.value,
    iambic: els.iambicSelect.value,
    adaptiveCoach: els.adaptiveCoachInput.checked,
    symbols: [...els.symbolGrid.querySelectorAll("input:checked")].map((input) => input.value)
  };
  localStorage.setItem(STORAGE_KEY, JSON.stringify(settings));
}

function setSymbolPreset(kind) {
  const values = kind === "koch" ? KOCH_START : kind === "letters" ? "ABCDEFGHIJKLMNOPQRSTUVWXYZ" : "0123456789";
  for (const input of els.symbolGrid.querySelectorAll("input")) {
    input.checked = values.includes(input.value);
    input.parentElement.classList.toggle("selected", input.checked);
  }
  newTarget();
}

function labelForMode(mode) {
  if (mode === "single") {
    return "Single";
  }
  if (mode === "word") {
    return "Word";
  }
  if (mode === "sentence") {
    return "Sentence";
  }
  return "Free";
}

function clamp(value, min, max) {
  return Math.min(max, Math.max(min, value));
}

function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

function bindUi() {
  document.addEventListener("keydown", onKeyDown);
  document.addEventListener("keyup", onKeyUp);

  document.getElementById("focusButton").addEventListener("click", () => {
    startAudio();
    els.targetPanel.focus();
  });

  document.getElementById("newTargetButton").addEventListener("click", newTarget);
  document.getElementById("clearLogButton").addEventListener("click", () => {
    state.attemptLog = [];
    saveSessionLog();
    render();
  });
  document.getElementById("kochButton").addEventListener("click", () => setSymbolPreset("koch"));
  document.getElementById("lettersButton").addEventListener("click", () => setSymbolPreset("letters"));
  document.getElementById("numbersButton").addEventListener("click", () => setSymbolPreset("numbers"));

  for (const input of [
    els.modeSelect,
    els.wpmInput,
    els.wordLengthInput,
    els.wordCountInput,
    els.iambicSelect,
    els.adaptiveCoachInput
  ]) {
    input.addEventListener("change", newTarget);
  }

  els.toneInput.addEventListener("change", render);
  document.getElementById("applyCoachButton").addEventListener("click", () => {
    const focus = analyzeSession().focus.map((stat) => stat.symbol);
    if (!focus.length) {
      return;
    }
    for (const input of els.symbolGrid.querySelectorAll("input")) {
      input.checked = focus.includes(input.value);
      input.parentElement.classList.toggle("selected", input.checked);
    }
    newTarget();
  });
}

applySettings();
buildSymbolGrid();
buildReferenceGrid();
state.attemptLog = loadSessionLog();
bindUi();
newTarget();
