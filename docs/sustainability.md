# Funding Symta — sustainability research

A solo-language-maintainer survival document.  Filed as research,
not as a business plan -- numbers are calibrations from comparable
projects, not commitments.

## The constraint

Bare-minimum funding to keep Symta development going.  Calibration
for a Netherlands resident as ZZP (eenmanszaak / self-employed)
working solo:

  - Health insurance (mandatory): ~€140/month.
  - Modest rent outside Amsterdam (Utrecht / Rotterdam / Eindhoven
    / Groningen): €800-1200/month for a 1-bedroom.
  - Utilities + groceries + transport + misc: €700-1300/month.
  - **Total monthly expenses: €1700-2700.**
  - Tax overhead (income tax + ZVW health-insurance contribution
    + own pension allocation): ~25-30 % effective on ZZP income
    after the standard deductions (zelfstandigenaftrek €3,750,
    startersaftrek €2,123 for first 3 years, MKB-winstvrijstelling
    13.31 %).
  - **Gross target for bare-minimum survival: ~€32-42 k/year**,
    or roughly €2 700-3 500/month gross.

Symta needs SOME monetary stream to avoid becoming "Nancy's
hobby that has to compete with a day job for attention".  The
aim is **a few months of runway, then a recurring stream that
covers basic costs**, not a startup outcome.

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

## Netherlands-specific opportunities

Being a Dutch resident reshapes the analysis significantly.  The
Netherlands has one of the most generous R&D tax-credit regimes
in Europe for solo developers, plus several culture / tech
funds with explicit "experimental tools" remits, plus a research
institution (CWI Amsterdam) that has literally hosted the birth
of major programming languages.

### NL-1.  **WBSO** -- the single most underrated solo-dev benefit

The Wet Bevordering Speur- en Ontwikkelingswerk is a national
R&D tax credit administered by RVO (Rijksdienst voor Ondernemend
Nederland).  For a ZZP doing technical innovation -- which Symta
clearly is -- WBSO refunds **a fraction of every R&D hour you
spend, capped at ~1 800 hours/year**.

  - For ZZP without employees: a **fixed deduction of ~€15 000**
    against income tax in the year if WBSO-approved (2025
    figure; check RVO for current).  Plus the "aftrek voor
    speur- en ontwikkelingswerk" of €14 045 if you log 500+
    R&D hours.
  - Application: twice a year (deadline 30 September for next
    January, 31 January for next quarter, etc.).  Online via
    RVO portal.  Decision in ~3 months.
  - Eligibility: technical-innovation work that ISN'T already
    state-of-the-art for the applicant.  Inventing a new
    language counts.  Building a new code generator counts.
    Routine bugfixing doesn't.
  - **Total annual tax benefit for a Symta-focused ZZP: €15-25 k**
    against income tax, recurring as long as the R&D work
    continues.

This is the highest-EV single Dutch-specific item.  It doesn't
generate revenue, it generates tax-side runway -- but at ~€20 k
annually it's nearly half a survival budget.  Apply for the
upcoming WBSO call regardless of every other strategy.

### NL-2.  NLnet is *Dutch*

NLnet is based in Amsterdam.  The NGI Zero Core / NGI Zero
Commons / NGI Zero PET calls are EU-funded but administered from
the Netherlands by Dutch staff.  Symta has a quiet home-field
advantage for two reasons:

  - **Lower communication friction.**  You can apply in English
    but follow up in Dutch.  Calls and meetups happen in NL.
  - **Cultural fit.**  NLnet's funded projects show a clear bias
    toward "small, auditable, sovereign infrastructure" -- a
    near-perfect description of Symta.

  - The most realistic single application: **NGI Zero Core,
    €30-50 k**, for "small, auditable self-hosted language
    runtime with modern tooling".  Approval rate is
    relatively high (~30-40 %) for technically-credible
    applications with clear deliverables.
  - Alternative angle: NGI Zero PET (privacy-enhancing tech)
    if Symta's small footprint is pitched as enabling on-
    device computation that doesn't leak data to cloud APIs.

  **Apply now, not later.**  Decision time is ~3 months.

### NL-3.  Stimuleringsfonds Creatieve Industrie

The Creative Industries Fund administers multiple grant lines
relevant to Symta + SoM:

  - **Toegepast Onderzoek** (Applied Research): €25-80 k for
    "research projects that push the design discipline forward".
    Symta's type-system roadmap + the AI-assisted-dev research
    angle FITS this brief.
  - **Gamefonds**: €15-45 k for game development with cultural,
    experimental, or technologically-innovative angles.  Spell
    of Mastery's "designer wrote her own Lisp to make this"
    angle is exactly what they fund.
  - **Open Oproep** calls happen 2-3 times/year on rotating
    themes.

  Application form is in Dutch; deliverables can be in English.
  Decisions in 2-3 months.

### NL-4.  Dutch Game Garden

Utrecht-based game-dev incubator with a workspace, mentorship
network, and intro-to-publisher pipeline.

  - **Bizz Bench / Incubator program**: 1-year program for
    early-stage game studios.  Workspace + mentorship + intros.
    Free or symbolic fee.  Doesn't directly fund, but plugs
    into the Dutch game industry.
  - Useful as a network multiplier when Spell of Mastery is
    in Phase 1 (release prep).

### NL-5.  CWI Amsterdam -- *probably not applicable*

CWI is the national CS research institute (where Python was born).
A visiting-researcher arrangement would be useful but CWI
typically wants PhD-equivalent published research as an entry
condition.  Set aside; revisit only if a researcher there
proactively reaches out after seeing the Symta work, which is
not a path to plan around.

### NL-6.  Tax structure choice -- ZZP vs Stichting

For year 1 with revenue under €60 k:

  - **ZZP / eenmanszaak** (sole proprietor): simplest.  Income
    flows through personal tax return.  WBSO, zelfstandigen-
    aftrek, startersaftrek, MKB-winstvrijstelling all available.
    BTW (VAT) registration required above €20 k turnover (KOR
    scheme exempts below €20 k).
  - **Stichting Symta** (foundation): a separate legal entity
    that can hold IP, receive grants, accept donations.  If
    approved as **ANBI** (algemeen nut beogende instelling),
    donations are tax-deductible for donors -- meaningful for
    GitHub Sponsors and corporate donors.  More overhead (~€500
    setup + annual filings) but cleaner separation of "the
    project" from "Nancy's personal income".

  The pragmatic structure for the first 1-2 years: stay ZZP,
  receive grants and consulting personally, register WBSO.
  Convert to BV (private limited company) only when annual
  revenue clears ~€60-70 k where the BV's flat rates start
  beating personal income tax brackets.

### NL-7.  Steam / itch.io tax considerations

  - Steam takes 30 %, deducts 30 % US withholding on royalties
    by default.  File a W-8BEN form via the Steamworks dashboard
    to claim the US-NL tax treaty rate (0 % on royalties).  This
    is a one-page tax-treaty claim -- always worth doing.
  - itch.io: 10 % platform fee, no US withholding (creator
    handles their own taxation).
  - Digital products sold to EU consumers: VAT MOSS/OSS scheme
    -- Steam handles this for you on their platform.  For
    itch.io / direct sales you can register for **OSS** via the
    Belastingdienst portal.  Below €10 k cross-border EU sales
    you can use NL VAT rates; above that, OSS uses the
    customer's country rate.

### NL-7b.  Credentials, age, identity -- what matters and what doesn't

Three concerns worth addressing directly before they distort the
analysis:

**Self-taught vs formally credentialed.**  In programming-language
work specifically, self-taught is the norm rather than the
exception.  Track record is the credential.  Symta (working
compiler, working type-system roadmap, working game on it,
committed perf history) demonstrates more technical depth than
most CS Masters theses ever do.  Reviewers for WBSO, NLnet,
and Stimuleringsfonds read your project artefacts, not your
CV.  When the application form asks about background, "10+
years of self-directed language design with the working compiler
and 25 k-LOC reference application available to inspect" is
exactly the kind of credential they're looking for.

  Some self-taught contemporaries in PL / systems work who
  built funded careers without conventional credentials:

  - **Salvatore Sanfilippo** (Redis): Italian high-school
    background, fully self-taught.  Redis funded by Pivotal,
    later Redis Labs.
  - **Anders Hejlsberg** (Turbo Pascal → C# → TypeScript):
    never finished his TU Denmark degree.
  - **Linus Torvalds** had a degree but Linux was a self-
    taught hobby project.
  - **DHH** (Rails): self-taught.
  - Most Lisp/Scheme implementers post-1990 are self-taught
    relative to their actual deliverables.

  These are the people whose work the funders cite as
  inspiration.  You're applying in the same lineage.

**Age 41.**  This is mid-career for language designers, not
late.

  - **Rich Hickey** was 42 when Clojure shipped.
  - **John McCarthy** was 31 when he invented Lisp, then kept
    contributing into his 80s.
  - **Tarn Adams** has been working on Dwarf Fortress for 20+
    years and is in his 50s; funded entirely by sales /
    Patreon.
  - **Andrew Kelley** started Zig in his 30s.
  - Bjarne Stroustrup, Guy Steele, Niklaus Wirth -- all did
    significant PL work into their 60s and beyond.

  "Young professional" grants don't exist in the NL grants
  Symta is targeting.  WBSO has no age limit.  NLnet has no
  age limit.  Stimuleringsfonds has no age limit.  The
  Dutch funding ecosystem reads merit-based; the application
  form doesn't ask your age.  This concern doesn't apply to
  the actual grant landscape.

**Identity.**  The Netherlands has strong legal protections
(Algemene wet gelijke behandeling) and the grant programs
listed don't ask about identity.  This is purely a question
of application merit.

If you want to layer identity-specific support on top:

  - **Trans tech communities** exist (Out in Tech, Lesbians
    Who Tech, Trans*H4CK) but they're mostly US-based
    networking events; useful for connections, rarely for
    direct funding.
  - **Refugee-from-Russia programs** exist in NL via Stichting
    Vluchtelingenwerk and similar; they fund settlement /
    integration, not technical R&D, so out of scope here.

  None of these channels are worth competing with the WBSO +
  NLnet + Stimuleringsfonds path in terms of euros per hour
  of effort.  Mentioned for completeness.

### NL-7c.  Credential-free funding sources worth knowing about

Beyond the three Dutch heavyweights:

  - **Sovereign Tech Fund** (Germany, but funds EU-wide):
    specifically targets "underfunded but critical FOSS
    infrastructure".  €100 k-300 k grants; long process
    (~6-12 months from application to funding) but excellent
    fit philosophically for Symta.  No credential requirement;
    they assess the project.  Application opens roughly
    quarterly.
  - **GitHub Maintainer Pilot / GitHub Funds**: explicitly
    funds OSS maintainers, no academic gate.  Tiers vary; some
    one-time $5-20 k, some recurring.
  - **OpenSSF** (Open Source Security Foundation) and
    **OpenJS Foundation**: security or JS-adjacent; less direct
    fit but the application processes are credential-blind.
  - **Open Collective Europe**: not a grant, a *fiscal host*.
    Lets you receive donations / grants as a "collective"
    without forming your own Stichting.  Eliminates a lot of
    legal overhead while still letting you channel money
    professionally.  Worth setting up early.
  - **Patreon**: still works as a direct creator-support
    platform; lower discovery than GitHub Sponsors but a
    different audience overlap.
  - **Ko-fi / Liberapay**: smaller-scale tip-jar alternatives;
    Liberapay specifically doesn't take a cut.

### NL-8.  Other Dutch funds + sources to watch

  - **NWO** (Dutch Research Council): KIC (Knowledge and Innovation
    Covenant) calls, sometimes funds language tooling research.
    Long-cycle (12+ months from application to funding).
  - **Eurostars** (EU R&D for SMEs): up to €500 k per project,
    but needs a partner consortium.  Out of scope for solo dev.
  - **Innovatiekrediet** (RVO): low-interest loan for R&D,
    €150 k-€10 M.  Loan, not grant -- mention for completeness.
  - **Provincial / municipal funds**: depending on which gemeente
    Nancy lives in.  Utrecht and Eindhoven have active tech-
    startup funds; Amsterdam less so for solo devs.
  - **Voordekunst** (cultural crowdfunding): tax-deductible for
    donors if the campaign is via an ANBI stichting.  €5-50 k
    typical campaigns.

## What to do FIRST (this week, if you wanted to)

Ordered by **euro-per-hour-of-effort** for an NL resident.  The
top three are NL-specific and dwarf everything else in expected
value.

1. **WBSO application** (4-6 hours of paperwork, ~€15-25 k/yr
   recurring tax credit).  Single highest-EV item in this entire
   doc.  Apply via the **RVO portal** (https://www.rvo.nl/) for
   the upcoming WBSO cycle.  The application describes your R&D
   project in plain Dutch / English; "developing a self-hosted
   AOT-compiled Lisp dialect with a novel type system and native
   code generation" is exactly the kind of work WBSO funds.  If
   you don't have a ZZP / eenmanszaak registered at KvK
   (Chamber of Commerce) yet, do that first (€80 + 30 minutes
   online).
   - Realistic outcome: ~€15-20 k/year reduction in income tax,
     for as long as the R&D continues.
   - This is recurring, not one-shot.

2. **NLnet pre-application email** (2 hours).
   The Dutch home-field advantage makes this the most realistic
   single grant.  Email contact@nlnet.nl with a one-page sketch:
   "Symta: small, auditable, self-hosted Lisp dialect with AOT
   compilation; type-system + native-code roadmap; AI-assisted
   development research substrate".  Reference the existing
   technical assets: working compiler, working game, existing
   documentation, recent perf work.  Ask whether the NGI Zero
   Core or NGI Zero Commons call is the better fit.
   - Realistic outcome: €30-50 k one-shot grant over 9-12 months
     for specific deliverables.

3. **Stimuleringsfonds Creatieve Industrie -- Toegepast Onderzoek
   pre-app** (3-4 hours).  Browse https://stimuleringsfonds.nl/
   for the current Open Oproep.  The "AI-assisted dev of
   self-hosted Lisp" research angle fits Toegepast Onderzoek;
   "ship a strategy game written in a custom language" fits
   Gamefonds.  Pick whichever has a closer deadline.
   - Realistic outcome: €25-80 k one-shot project funding.

4. **`.github/FUNDING.yml`** (15 minutes).
   ```yaml
   github: [NancyAurum]   # or your actual GH handle
   ko_fi: nancyaurum
   custom: ["https://nancygold.itch.io/edds"]
   ```
   Zero risk, immediate "tip jar" availability.  Counts as the
   "Sponsors" link on the GitHub repo automatically.

5. **README.md "Sponsors / Funding" section** (30 minutes).
   One paragraph above the License heading pointing at GitHub
   Sponsors + itch.io + (later) Steam.  Converts existing repo
   traffic into trickle income.

6. **HN-ready blog post submission** (1 day).
   The 765-line blog.md is too long for HN.  Cut it down to
   1500-2500 words focused on the most novel angle ("language
   designer + LLM = revived 25 k LOC game").  Submit to HN +
   lobste.rs + /r/programming.  Time it for Tuesday / Wednesday
   morning EU time when HN is most active.  Drives
   sponsor / consulting visibility.

7. **Spell of Mastery itch.io page polish** (1 week part-time).
   The page exists at https://nancygold.itch.io/edds.  The
   marketing copy, screenshots, trailer need attention.  itch.io
   revenue activates within hours of going live; Steam takes
   weeks of pre-launch.

8. **Sovereign Tech Fund pre-application** (2 hours, deferred 1-2
   months).  Read https://www.sovereigntechfund.de/ application
   criteria.  Symta fits their "underfunded but critical FOSS
   infrastructure" mandate philosophically, although they
   typically want a project that's already in use by others.
   Save this for after the SoM launch + HN post when Symta has
   external traction to cite.  €100-300 k grants are worth the
   wait.

9. **Open Collective Europe fiscal host setup** (1 hour).
   Lets you receive donations and small grants under a
   "collective" umbrella without forming your own Stichting
   yet.  Eliminates legal overhead while still letting the
   project receive money professionally.  Convert to Stichting
   only if grant volume justifies it (probably year 2+).

Items 1-3 are the NL-specific high-impact moves.  Items 4-5
take an hour total and start the visibility flywheel.  Items
6-7 are content-driven and lead-generate for everything else.
Items 8-9 are infrastructure for year 2.

Total weekend of effort to get items 1-5 in motion.  WBSO + NLnet
+ Stimuleringsfonds together could provide €40-100 k of runway
in year 1 if even half land -- the difference between "Nancy's
hobby" and "Symta is Nancy's job".

**None of these require credentials.**  WBSO assesses the
technical work, NLnet assesses the project, Stimuleringsfonds
assesses the proposal.  The committed Symta + SoM artefacts are
exactly the kind of evidence these processes want to see.

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

**File WBSO + NLnet + Stimuleringsfonds applications this month,
ship SoM as the medium-term funding engine, set up the tip jar
+ HN blog post in parallel for visibility, and use whichever of
those lands first to fund the runway for whichever lands next.**

For a Dutch resident, the WBSO tax credit alone is structurally
worth ~€15-25 k/year recurring -- nearly half a survival budget,
just for filing the paperwork.  Combined with one Dutch grant
landing and a modest Spell of Mastery launch, the bare-minimum
target is within reach in year 1.

The visibility-driven paths (sponsors, book, consulting) are
year-2 multipliers, not year-1 survival.  The Dutch
institutional landscape is generous to solo R&D devs in ways
most countries aren't; use it.
