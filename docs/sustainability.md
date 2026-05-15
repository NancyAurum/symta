# Funding Symta — sustainability research

A solo-language-maintainer survival document.  Filed as research,
not as a business plan -- numbers are calibrations from comparable
projects, not commitments.

## The constraint

Bare-minimum funding to keep Symta development going.  Calibration:
a Polish / Russian / Latvian cost-of-living single-developer
threshold is ~€1.5-2 k/month; a Berlin / Amsterdam threshold is
~€3-4 k/month.  Symta needs SOME monetary stream to avoid
becoming "Nancy's hobby that has to compete with a day job for
attention".  The aim is **a few months of runway, then a recurring
stream that covers basic costs**, not a startup outcome.

## What Symta uniquely has as monetisable assets

Most language projects have one or zero of these.  Symta has four:

1. **The language itself** -- working, self-hosted, AOT-compiled,
   small C runtime, BSD-license-equivalent (dual MIT / Apache 2).
   Few people on Earth can build this; languages take 5-10 years
   to reach this state.
2. **Spell of Mastery** -- a real ~25 k-LOC turn-based strategy
   game written ENTIRELY in Symta.  Already has an
   [itch.io](https://nancygold.itch.io/edds) presence under
   Nancy's other project.  Working binary in the repo.  Proven
   demo that Symta is shippable, not vapourware.
3. **Long-form technical content** -- the 765-line blog.md about
   reviving SoM with AI-assisted dev is genuinely interesting
   reading.  AI-coding is the topic of the moment; "language
   designer revives 25 k LOC game in her own custom Lisp with an
   LLM" is HN front-page material.
4. **Domain expertise** -- compilers, GC, FFI, GUI toolkits,
   software rasterisation.  Each of those individually is a
   well-paid consulting specialty.  Stacked, very rare.

Few solo developers have any one of these.  Stacking them is the
foundation of any sustainability strategy.

## Survey of paths -- with comparable-project numbers

Each path below is backed by a real example.  Numbers are
public-facing estimates (GitHub Sponsors, Steam revenue,
publisher statements) -- treat as order-of-magnitude.

### Tier 1: paths that have shipped solo-dev language projects

**G1.  Game sales as the funding engine for the language.**
The Stardew Valley / Caves of Qud / Dwarf Fortress pattern: a
working game funds everything the developer needs, including
the unusual tools they invented to build it.

  - **Stardew Valley** (Eric Barone): $30M+ over lifetime, solo
    dev, custom-built on XNA / MonoGame.
  - **Caves of Qud** (Brian Bucklew / Jason Grinblat): ~$5-10M
    over a decade, custom Mercury engine, ~$30 base price.
  - **Dwarf Fortress** (Tarn Adams): ~$8M Steam launch month +
    ~$60k/month Patreon.

  Spell of Mastery is a closer cousin of Caves of Qud than of
  Stardew: turn-based, strategy / RPG-flavoured, the developer's
  language tool stack IS part of the unique pitch.  A polished
  Steam release at $15-25 + an itch.io page can plausibly
  fund Symta development for years on its own.  THIS IS THE
  HIGHEST-EV PATH for a solo dev with a complete-ish game in
  hand.

**G2.  GitHub Sponsors / Patreon / Open Collective.**
Pure donation-driven recurring revenue.  Works ONLY at
visibility-scale.

  - **Andrew Kelley (Zig):** ~$190 k/year via Zig Software
    Foundation + sponsors.  Took 7 years of public work to
    reach this floor.
  - **Sindre Sorhus:** ~$25 k/month, but he has 1000+ npm
    packages with billions of weekly downloads.  Unique scale.
  - **Drew DeVault (sourcehut):** ~$5 k/month before sourcehut
    became a paid service.

  Realistic Symta floor pre-visibility: $50-200/month.  After
  a HN front-page + a few targeted talks: $500-1500/month is
  feasible.  After several years of public work: ~$2-3k/month.
  Slow-ramp, low-effort-to-maintain, but visibility-bottlenecked.

**G3.  Foundation grants.**
Project-based one-shot funding.  Symta's strongest grant angles:

  - **NLnet** (EU, the most realistic fit): €5 k-50 k grants
    for foundational FOSS infrastructure.  Symta's "small,
    auditable language runtime" angle matches their NGI Zero
    Core call.  Also potentially their NGI Search call if we
    spin the GPU / tensor angle.  Apply Q1 / Q3; decisions in
    ~3 months.
  - **Sovereign Tech Fund** (Germany): larger grants for
    "critical digital infrastructure" -- Symta is too obscure
    today, but if it gets adopted in a critical workflow, the
    angle becomes real.
  - **NumFOCUS** (scientific Python ecosystem): only relevant
    if Symta's tensor / GPU story lands AND we partner with a
    research group.
  - **Mozilla MOSS / Mozilla Open Source Support:** dormant
    in 2024-2026 but watch for revival.
  - **GitHub Accelerator / Google Summer of Code:** mentor /
    student work; not direct funding but a way to grow the
    contributor base.

  NLnet is the most realistic single grant in the next year.
  €30 k buys ~6-9 months of focused work.

### Tier 2: paths with longer ramp

**B1.  Book.**
The "Crafting Interpreters" lineage.  One-time writing effort,
years of long-tail income.

  - **Bob Nystrom (*Crafting Interpreters*):** ~$70-100 k/year
    in royalties, 5+ years after launch.  Self-published.
  - **Thorsten Ball (*Writing an Interpreter in Go*):**
    similar ballpark.  Two books in the series; second one
    written part-time on book-1 income.
  - **Ian Lance Taylor / various Go books:** $20-40 k each.

  Symta is a great book subject -- the compiler is small enough
  to walk through end-to-end, it's self-hosted (so the book IS
  the explanation of the language by which the book exists),
  and the AI-assisted-dev angle is contemporary.  Working title:
  *Crafting a Concise Lisp*.

  Effort: 6-12 months part-time.  Output: a $35 PDF + $50 paper
  edition.  Plausible Year 1: ~$5-15 k.  Year 2-5: $10-30 k/yr.

**B2.  Consulting / contracting.**
Per-engagement.  Symta becomes the portfolio piece, not the
product.

  - Rates for language / compiler work: $200-400/hour.
  - Recurring monthly retainer with one mid-sized employer:
    $4-8 k/month for 1-2 days/week.
  - The hard part: finding clients.  Lead-time is long.

  Symta's working game + the AI-assisted-dev blog are great
  lead generators IF you actively pursue this.  Most language
  experts get consulting work via talks, papers, or referrals
  from existing networks.

**B3.  Educational / academic.**
Universities pay people to teach programming languages.

  - Visiting researcher / postdoc-level: €40-60 k/year, often
    1-2 year contract.
  - Adjunct teaching (PL course at a CS dept): €5-10 k per
    semester course, 2-3 courses/year max for part-time.
  - PhD funding: €15-25 k/year stipend, 3-4 years; opens path
    to permanent positions.

  Mathematically lowest-risk, highest-floor / lowest-ceiling
  path.  Requires academic affiliation, conference attendance,
  publishing.  Major time commitment to non-Symta work.

### Tier 3: long-tail / opportunistic

**T1.  Commercial dual-licensing tier.**
Current MIT / Apache 2 makes this hard -- you'd need to relicense
or split off a commercial product line.  Two options:

  - **Relicense everything to GPL** + sell commercial exception
    licences to companies that need permissive redistribution.
    This is Sidekiq / Qt's model.  Risk: alienates existing
    community.
  - **Keep open core MIT/Apache** + sell commercial-only
    add-ons: AOT compiler, GPU back-end, debugger, IDE.
    Lower friction; lower revenue ceiling.

  Realistic income: $0-10 k/year initially.  Scales only with
  user base.  Not viable until Symta has 100+ active users.

**T2.  Game-engine licensing.**
UIM + cls + the rendering pipeline + Symta-as-scripting could
be sold as a niche game framework, analogous to Lua-as-scripting
for many engines.  $50-200/game-developer-seat / year.

  Realistic income: $0 in year 1.  Requires marketing
  investment + a real "studio adopts Symta" anchor story.
  Probably premature.

**T3.  SaaS / hosted dev tools.**
Replit-style cloud IDE for Symta; hosted compile-and-share;
playground.  Recurring revenue but huge ongoing infra cost for
solo dev.  Skip until Symta has critical mass.

**T4.  Hardware tie-in.**
Symta-on-embedded for a specific niche (sensor data processing,
on-device DSP).  Speculative; requires hardware partner.

## Recommended strategy: a combination, sequenced

No single path covers a sustainability target on its own.  The
realistic shape is a **portfolio**: one quick-revenue stream
funds the ramp on a slower-but-larger stream.

**Phase 0 (next 2 weeks, near-zero effort).**
  - Set up `.github/FUNDING.yml` pointing at a GitHub Sponsors
    profile.  Three tiers: $3/mo (logo in README acknowledgements),
    $25/mo (early access to internal-progress blog), $100/mo
    (1 hour/month language design Q&A).
  - Write a concise itch.io / Steam game page for Spell of
    Mastery using the existing blog.md content as marketing.
  - File the NLnet application (NGI Zero Core call) with the
    "small auditable language runtime" angle + the AI-assisted
    dev story.

**Phase 1 (next 2-3 months).**
  - Polish + ship Spell of Mastery on itch.io + Steam.
  - Submit the AI-assisted dev blog post (or a sharpened
    excerpt) to HN front page.  This drives initial sponsor
    traction.
  - First conference talk: "What I learned re-writing 25 k LOC
    of my own Lisp dialect with an LLM" -- Strange Loop,
    LangCon, FOSDEM, Lisp / Scheme Workshop.  Recordings have
    long tail.

**Phase 2 (3-9 months).**
  - Consulting: announce availability for language / compiler
    consulting on the blog + on the GitHub profile.  Even 1-2
    engagements / year covers a significant chunk of the bare-
    minimum target.
  - Book proposal: "Crafting a Concise Lisp" -- pitch to
    pragprog / no-starch / self-publish via Leanpub.

**Phase 3 (year 2+).**
  - Book launches.  Royalty stream starts compounding.
  - Spell of Mastery DLC / expansion.
  - Symta v1.0 freeze + the NATIVE roadmap from TODO.md lands;
    Symta is now a credibly fast language, not a research toy.
    Commercial licensing tier becomes viable for companies that
    embed it.

## What to do FIRST (this week, if you wanted to)

Ordered by best-effort-per-hour:

1. **`.github/FUNDING.yml`** (15 minutes).
   ```yaml
   github: [NancyAurum]   # or whichever GitHub handle
   ko_fi: nancyaurum
   custom: ["https://nancygold.itch.io/edds"]
   ```
   Zero risk, immediate "tip jar" availability.

2. **README.md "Sponsors" section** (30 minutes).
   Add a one-paragraph section above the License heading
   pointing at GitHub Sponsors + itch.io + (later) Steam.

3. **HN-ready blog post submission** (1 day).
   The 765-line blog.md is too long for HN.  Cut it down to
   1500-2500 words focused on the most novel angle ("language
   designer + LLM = revived 25 k LOC game").  Submit to HN +
   lobste.rs + /r/programming.  Time it for Tuesday/Wednesday
   morning EU time when HN is most active.

4. **NLnet pre-application email** (2 hours).
   NLnet welcomes pre-applications.  Sketch the angle:
   "Symta: small, auditable, self-hosted Lisp dialect with
   AOT compilation -- a research substrate for AI-assisted
   language design and embedded scripting".  See if they want
   a full application.

5. **Spell of Mastery itch.io page polish** (1 week part-time).
   The page exists; the marketing copy, screenshots, trailer
   need attention.  itch.io revenue can be activated quickly
   compared to Steam.  Use the existing blog screenshots.

The first four items together are maybe 2 working days.  Even
a modest result on any one of them is more income than the
project has today.

## Risks + open questions

- **Spell of Mastery completion percentage.**  Phase 1 hinges
  on the game being ship-ready.  How close to ship is it?  If
  it's 6 months from release, the strategy compresses Phase 1
  into Phase 2 -- still works.  If it's a year+ away, Phase 0
  + grants need to do more work in the meantime.

- **Time budget.**  All strategies above assume Nancy has at
  least 20-30 hours/week to dedicate.  If the actual budget is
  10 hours/week, the realistic targets shrink ~3×.  The "what
  to do first" list still applies.

- **Visibility bottleneck.**  Sponsorship + grants + book sales
  all depend on Symta being discoverable.  HN + lobste.rs +
  programming language conferences are the standard channels.
  Conference talks have outsized impact: one good Strange Loop
  talk has launched several careers (`tweag.io`, `fly.io` early
  hires came from there).

- **Risk-tolerance.**  Game development is bursty.  Grants are
  one-shot.  Consulting is sporadic.  Sponsorships are recurring
  but small.  A diversified portfolio reduces the chance of
  any single failure ending the project; it also caps the
  upside of any single success.

- **Personal brand.**  "Nancy Sadkov / Nancy Aurum / nancygold"
  is currently scattered across handles + platforms.
  Consolidating under one professional identity (one website,
  one Twitter / Mastodon, one GitHub, one itch.io) raises
  the floor of every income stream.  ~1 weekend of work.

## My recommendation in one sentence

**Set up the tip jar this week, ship SoM as the funding engine,
file the NLnet application as the bridge funding, and let the
blog content drive the visibility flywheel that makes
everything else viable.**

That's the shape that has worked for a half-dozen comparable
solo-language projects in the last decade; Symta has the assets
(working game, working language, current AI-coding story) to
play the same hand better than most.
