# Symta — PR & funding strategy

The consolidated action plan.  Distilled from the research in
[`docs/sustainability.md`](docs/sustainability.md) into something
you can actually execute against.

## Bottom line in two sentences

**File the WBSO + NLnet + Stimuleringsfonds Creatieve Industrie
applications this month**; that combination alone covers the
bare-minimum NL survival budget if even two of the three land.
**Ship Spell of Mastery on itch.io + Steam in parallel** as the
medium-term funding engine; let the work stand on its technical
merits without leaning on backstory.

## Context

- **Resident of the Netherlands.**  ZZP / eenmanszaak status is
  the right tax structure for year 1-2.  Convert to BV only past
  ~€60-70 k revenue.  Stichting only if grant volume justifies
  the overhead (year 2+).
- **Bare-minimum monthly budget.**  Expenses €1.7-2.7 k/month,
  effective tax overhead ~25-30 % on ZZP income after deductions.
  **Gross target: €32-42 k/year**, ~€2.7-3.5 k/month gross.
- **Solo developer, self-taught.**  No academic credentials.
  Mid-career (41).  Working compiler + 25 k-LOC reference
  application in the repo.  10+ years of self-directed language
  design.

## What Symta uniquely has

Four monetisable assets that almost no other solo-language project
stacks at once:

1. **The language itself.**  Working, self-hosted, AOT-compiled,
   small C runtime, dual MIT / Apache 2 license.  Recent perf
   work brought game cold compile from 51-54 s to 20-23 s; the
   TYPE-* + NATIVE-* roadmap in `TODO.md` targets "near C99
   latency" on numeric workloads.
2. **Spell of Mastery.**  ~25 k-LOC turn-based strategy game
   written entirely in Symta.  Already on itch.io.  Working
   binary in the repo.  Closer cousin of Caves of Qud than of
   Stardew: turn-based / strategy / RPG-flavoured, unique tool
   stack IS part of the pitch.
3. **Long-form technical content.**  The 765-line `blog.md`
   about reviving SoM with AI-assisted development is genuinely
   HN front-page material.  Timely topic + concrete results +
   unusual angle.
4. **Domain expertise.**  Compilers + GC + FFI + GUI toolkits +
   software rasterisation.  Each one a high-rate consulting
   specialty.  Stacked, very rare.

Most solo developers have zero or one of these.  Stacking the
four is the foundation of every strategy below.

## Positioning principle: the work stands alone

The biography, the path, the personal history — all real, all
yours — stay out of public-facing Symta materials.

**Three reasons:**

1. **The funders in our shortlist don't ask.**  WBSO assesses
   technical innovation, NLnet assesses the project, Stimulerings-
   fonds assesses the proposal, Sovereign Tech Fund assesses
   strategic value to FOSS infrastructure, Steam asks nothing,
   GitHub Sponsors / Patreon care about output.  None of them
   read "education history" before deciding.
2. **Mixing backstory into the pitch dilutes both.**  When the
   pitch is "here's a working language with concrete deliverables
   and roadmap", reviewers can evaluate it on merit.  When the
   pitch is "underdog story + working language", the language
   becomes part of the underdog story and gets evaluated more
   loosely — both ways: some funders soften their technical bar,
   others raise their suspicion bar.  Neither is good.
3. **It puts a ceiling on the work.**  Once Symta is "the
   language by the [story]", every subsequent piece of coverage
   is filtered through that lens.  The "diversity / underdog
   tech" bucket is real, it's lower-resourced, and it's hard to
   leave once you're in it.  Caves of Qud / Dwarf Fortress /
   Clojure / Redis didn't get famous for their authors'
   backgrounds.  They got famous for the work.

**Where the personal narrative *does* belong:**

- Memoir or non-technical book, written later in the arc after
  Symta has technical reputation of its own.  The memoir lands
  harder *because* the work stood up first.
- A single thoughtful blog post, when it makes sense to write
  one, not as a funding pitch.
- Specific community spaces when you choose to be in them.
- Conversations with people you trust.

**Where it doesn't:**

- WBSO / NLnet / Stimuleringsfonds applications.
- Steam / itch.io game-store pages.
- Hacker News submissions about Symta.
- Conference talks about the compiler.
- Book proposals for the technical book.
- Consulting pitches.
- README.md.

**Default answer when credentials come up:**  *"I'm self-taught;
here are the artefacts."*  No defence, no apology, no biography.
Same answer Salvatore Sanfilippo (Redis), Anders Hejlsberg (C# /
TypeScript, never finished his degree), Linus Torvalds, DHH
(Rails), most Lisp / Scheme implementers post-1990 give.  You
are applying in the same lineage and you have the same kind of
answer they have.

If a reviewer pushes: *"I started building programming systems
young and chose to keep building rather than going through the
formal track.  The work is the path."*  Said with confidence,
not apology.

## Funding paths — the portfolio

Three tiers ordered by realistic EUR-per-hour of effort given
your circumstances.

### Tier 1 — file this month, biggest expected value

**WBSO** (R&D tax credit, RVO administered).  Refunds ~€15-25 k
per year against income tax for a ZZP doing technical innovation.
Building a self-hosted Lisp with a novel type system + native
code generation is exactly what they fund.  Recurring as long
as R&D continues.  Application is 4-6 hours of paperwork via
the RVO portal.

  - Prerequisite: KvK-registered ZZP / eenmanszaak.  €80 + 30
    minutes online at https://www.kvk.nl/.
  - Application: https://mijn.rvo.nl/wbso.
  - Decision: ~3 months.
  - Realistic outcome: **€15-25 k/year recurring tax credit**.

**NLnet** (NGI Zero Core or NGI Zero Commons).  EU-funded,
Amsterdam-administered.  Quiet home-field advantage for a Dutch
applicant with a clear FOSS infrastructure angle.  Reviewers
read project artefacts.

  - Pre-application email: contact@nlnet.nl.  One-page sketch:
    "Symta: small, auditable, self-hosted Lisp dialect with AOT
    compilation; type-system + native-code roadmap; AI-assisted
    development research substrate".  Reference the working
    compiler, the SoM game, the blog content, recent perf work.
  - Decision: ~3 months.
  - Realistic outcome: **€30-50 k one-shot grant** over 9-12
    months for specific deliverables.

**Stimuleringsfonds Creatieve Industrie** (Creative Industries
Fund).  Dutch national culture fund.  Explicitly funds self-
taught makers; no academic gate.  Three lines apply to Symta /
SoM:

  - **Toegepast Onderzoek** (applied research, €25-80 k):
    "research projects that push the design discipline forward".
    Type-system roadmap + AI-assisted dev angle fits.
  - **Gamefonds** (€15-45 k): game development with cultural,
    experimental, or technologically-innovative angles.  SoM's
    "designer wrote her own Lisp" angle is exactly what they
    fund.
  - **Open Oproep** rotating calls: check stimuleringsfonds.nl
    for the current theme.

  Application is in Dutch; deliverables can be in English.
  Decision: 2-3 months.

**Combined realistic outcome of WBSO + NLnet + Stimuleringsfonds:**
**€40-100 k of runway in year 1 if even half of them land** —
the difference between hobby and job.

### Tier 2 — start now, pays in 6-12 months

**Spell of Mastery launch.**  The Caves-of-Qud-cousin pattern.
A polished release at $15-25 on Steam + itch.io can plausibly
fund the language for years on its own.

  - itch.io activates faster (hours).  Use the existing
    https://nancygold.itch.io/edds page.  Polish marketing copy,
    screenshots, trailer.
  - Steam launch in Q2-Q3 after itch.io traction provides social
    proof.  File **W-8BEN** in Steamworks dashboard to claim
    US-NL tax treaty rate (0 % royalty withholding) before any
    revenue.
  - VAT: Steam handles OSS for you.  itch.io direct sales above
    €10 k cross-border require OSS registration via Belastingdienst.

**GitHub Sponsors + Open Collective Europe.**  Set up the tip
jar.  Open Collective Europe as fiscal host means you can
receive donations and small grants under a "collective" umbrella
without forming a Stichting yet.

  - `.github/FUNDING.yml` in 15 minutes.
  - Open Collective signup in 1 hour.
  - Realistic floor: €50-200/month initially; €500-1500/month
    after HN traction + a few conference talks.

**HN-ready blog post.**  Cut `blog.md` down to 1500-2500 words
on the most novel angle ("language designer revives 25 k LOC of
own Lisp with LLM assistance").  Submit to HN + lobste.rs + r/
programming.  Tuesday / Wednesday EU morning maximises HN
exposure.  Drives sponsor + consulting + grant-reviewer
visibility.

### Tier 3 — year 2+, compound paths

**Book.**  *Crafting a Concise Lisp* or similar.  The
Crafting-Interpreters / Writing-an-Interpreter-in-Go lineage.
6-12 months part-time to write.  Year 1: €5-15 k.  Years 2-5:
€10-30 k/year long tail.

**Consulting.**  Announce availability after SoM launch +
HN post.  Even 1-2 engagements per year at €200-400/hour covers
significant runway.  Lead-time is months; start early.

**Sovereign Tech Fund** (Germany, EU-wide).  Targets
"underfunded but critical FOSS infrastructure".  €100-300 k
grants.  Save for after SoM launch + HN traction when Symta has
external usage to cite.

**Commercial extras** (year 2+).  Open core MIT/Apache + paid
add-ons: AOT compiler licence for embedded use, GPU back-end,
debugger, IDE.  Viable only after a user base exists.

## What to do this week (~2 working days total)

Ordered by EUR-per-hour for an NL resident with current
circumstances.

1. **Register ZZP at KvK** if not already (~€80, 30 minutes
   online).  Prerequisite for WBSO; needed for any
   independent contracting income anyway.

2. **WBSO application via RVO portal** (~4-6 hours).  Single
   highest-EV item in this entire document.  ~€15-25 k/year
   recurring tax credit.  The application describes your R&D
   project in plain Dutch or English; "developing a self-hosted
   AOT-compiled Lisp dialect with a novel type system and native
   code generation" is the brief.  Reference the committed
   roadmap in `TODO.md`.

3. **NLnet pre-application email** (~2 hours).  contact@nlnet.nl.
   One-page sketch + technical references.  Ask which call (NGI
   Zero Core vs NGI Zero Commons vs NGI Zero PET) is the best
   fit before doing the full application.

4. **Stimuleringsfonds pre-application review** (~3-4 hours).
   Read the current Open Oproep / Toegepast Onderzoek /
   Gamefonds calls.  Pick whichever has the closest deadline
   and the best brief fit.  Outline the project.

5. **`.github/FUNDING.yml` + README "Sponsors" section** (~45
   minutes).
   ```yaml
   # .github/FUNDING.yml
   github: [NancyAurum]   # or your GH handle
   ko_fi: nancyaurum
   liberapay: nancyaurum
   custom: ["https://nancygold.itch.io/edds"]
   ```

6. **Open Collective Europe setup** (~1 hour).  Create a
   collective for Symta as fiscal host.  Receive donations
   and small grants without forming a Stichting yet.

7. **HN-ready cut of `blog.md`** (~1 day).  1500-2500 words on
   the AI-assisted-dev / Symta-revival angle.  Schedule for
   Tuesday / Wednesday EU morning submission.

8. **Spell of Mastery itch.io page polish** (~1 week part-time).
   Update marketing copy, take fresh screenshots, ideally a 60-
   second trailer.  Use blog screenshots where production-
   trailer effort is too much.

Items 1-6 are 1-2 working days.  Item 7 is one focused day.
Item 8 spans a week alongside everything else.

## Bootstrap timeline

**Month 1 (now → mid-next-month):**
  - WBSO + NLnet + Stimuleringsfonds applications filed
  - FUNDING.yml + Open Collective live
  - HN blog post submitted
  - SoM itch.io page revised
  - **Outcome:** ~€0-500 immediate (tip jar) + applications in
    the pipeline

**Months 2-4:**
  - SoM polish toward Steam launch
  - First conference-talk pitch (Strange Loop, FOSDEM, Lisp /
    Scheme Workshop)
  - WBSO decision (Q1 application → Q2 decision typical)
  - NLnet decision (~3 months)
  - Stimuleringsfonds decision (~2-3 months)
  - **Outcome:** WBSO active (effective ~€1.5-2 k/month tax
    reduction); grant decisions arriving

**Months 4-9:**
  - SoM Steam launch
  - Continued blog content cadence (1 substantial post/month)
  - Sovereign Tech Fund pre-application
  - Book proposal drafted
  - **Outcome:** game revenue starts; total monthly stream
    approaching bare-minimum target

**Months 9-12 (end of year 1):**
  - Symta technical roadmap visibly progressing (OP-* +
    RT-4 + TYPE-1..3 landing)
  - First consulting engagement closed (if pursuing)
  - **Outcome:** stable monthly income at bare-minimum band

**Year 2+:**
  - SoM DLC / second project
  - Book launch
  - TYPE-4 + NATIVE-PRE land; commercial licensing tier
    becomes viable
  - Symta has 100+ active users; ecosystem effects begin

## Side concerns to handle separately

- **Dutch language certification (NT2).**  Don't wait on the
  refugee-stream school.  Alternatives: Volksuniversiteit adult
  NT2 (€200-500/year), online NT2 (Studiekring, NTI), private
  tutor (€30-50/hour).  If refugee status applies, **DUO
  inburgering loan** covers up to €10 k of NT2 at any accredited
  provider.  English is sufficient for almost all the funding
  paths above; NT2 is a separate life concern, not a funding
  blocker.

- **Identity-related infrastructure** (healthcare, legal name /
  gender registration, social support).  All available in NL via
  standard channels; not in scope for this document.

## Risks + open questions

- **SoM completion percentage.**  Phase 2 of the timeline
  assumes SoM is ship-ready by ~Month 4.  Honest assessment
  needed: how many features-from-the-old-design need to ship
  before a launch, vs how much of the existing codebase is
  ship-ready as-is?  An MVP-on-itch.io launch at Month 2 is
  better than a polished-on-Steam launch at Month 12 from a
  revenue-now standpoint.

- **Time budget.**  Timeline above assumes 25-30 hours/week on
  Symta + SoM combined.  If actual budget is 10-15 hours/week,
  every milestone shifts ~2× and the bare-minimum target
  becomes harder to hit in year 1.

- **Visibility flywheel.**  HN front page is hit-or-miss.  A
  failed first submission isn't fatal but slows everything that
  depends on it (sponsors, conference invites, consulting
  leads).  Plan: have 2-3 distinct posts ready so one can be
  re-tried at different angles if needed.

- **NLnet rejection risk.**  ~30-40 % approval rate is good but
  not certain.  Mitigation: re-apply in the next cycle with
  feedback incorporated.  Stimuleringsfonds is a fallback.

- **WBSO definition tightening.**  RVO occasionally tightens
  what counts as R&D.  Symta clearly qualifies today; document
  the application carefully and keep R&D hours logged.

- **One scattered online identity.**  Nancy Sadkov / Nancy
  Aurum / nancygold across GitHub, itch.io, etc.  Consolidating
  under one professional identity (one site, one GitHub, one
  itch.io, one Mastodon / Twitter) raises every income stream's
  floor.  ~1 weekend of housekeeping.

## Closing principle

The math works.  WBSO alone (~€18 k/year effective benefit
average) plus one Dutch grant landing (€25-50 k one-shot,
amortise over 12-18 months ≈ €25 k/year) plus a modest SoM
launch puts year 1 in the bare-minimum band without selling a
single book, attracting a single sponsor, or closing a single
consulting deal.  Everything else is upside.

The technical work is genuinely excellent.  The Dutch
institutional landscape is generous to solo R&D developers in
ways most countries aren't.  The funders in the shortlist
assess project artefacts, not biography.  Lead with the work.
