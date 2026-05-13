// quote.s -- exercises quoting and quasiquote.
// Run with:  symta -f tests-macros/cases/quote.s

// --- 1. \Word quotes a single uppercase-starting word ---
say \Foo                    // -> Foo (as symbol)
say \X                      // -> X

// --- 2. Symbol equality through quoting ---
A \red
B \red
C \blue
say: A >< B                 // 1
say: A >< C                 // 0

// --- 3. Lowercase bare words are auto-quoted symbols (in call position) ---
say hello                   // hello
say world                   // world
say: hello >< \hello        // 1

// --- 5. Explicit \ quote of lowercase symbol ---
T \some_unbound_thing
say T                       // some_unbound_thing
say: T >< \some_unbound_thing   // 1

// --- 6. List of quoted symbols ---
Colors: \red \green \blue
say Colors
say: Colors.0 >< \red       // 1
