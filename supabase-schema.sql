create table if not exists public.paddle_profiles (
  profile_hash text primary key,
  settings jsonb not null default '{}'::jsonb,
  attempt_log jsonb not null default '[]'::jsonb,
  updated_at timestamptz not null default now()
);

alter table public.paddle_profiles enable row level security;

revoke all on public.paddle_profiles from anon;
revoke all on public.paddle_profiles from authenticated;

create or replace function public.paddle_pull_profile(p_profile_hash text)
returns table (
  settings jsonb,
  attempt_log jsonb,
  updated_at timestamptz
)
language sql
security definer
set search_path = public
as $$
  select
    paddle_profiles.settings,
    paddle_profiles.attempt_log,
    paddle_profiles.updated_at
  from public.paddle_profiles
  where paddle_profiles.profile_hash = p_profile_hash
  limit 1;
$$;

create or replace function public.paddle_save_profile(
  p_profile_hash text,
  p_settings jsonb,
  p_attempt_log jsonb
)
returns void
language plpgsql
security definer
set search_path = public
as $$
begin
  insert into public.paddle_profiles (
    profile_hash,
    settings,
    attempt_log,
    updated_at
  )
  values (
    p_profile_hash,
    coalesce(p_settings, '{}'::jsonb),
    coalesce(p_attempt_log, '[]'::jsonb),
    now()
  )
  on conflict (profile_hash) do update set
    settings = excluded.settings,
    attempt_log = excluded.attempt_log,
    updated_at = now();
end;
$$;

grant execute on function public.paddle_pull_profile(text) to anon;
grant execute on function public.paddle_pull_profile(text) to authenticated;
grant execute on function public.paddle_save_profile(text, jsonb, jsonb) to anon;
grant execute on function public.paddle_save_profile(text, jsonb, jsonb) to authenticated;
