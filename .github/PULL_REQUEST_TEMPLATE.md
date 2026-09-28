<!-- Explains the purpose of this pull request template without appearing in the rendered PR. -->
## Summary

<!-- Ask for the smallest factual description of what changed and why. -->
Describe the change and its reason.

<!-- Separate verification evidence from implementation description. -->
## Verification

<!-- Keep only commands that were actually executed. -->
- [ ] Relevant SultanC build completed.
- [ ] Relevant compiler/runtime checks completed.
- [ ] Both supported targets were considered when the change is target-sensitive.
- [ ] No tracked generated or build output was unintentionally committed.

<!-- Make architectural ownership visible during review. -->
## Architecture

<!-- Reject accidental duplicate ownership or backend target-policy leaks. -->
- [ ] The change modifies the canonical owner rather than adding a parallel implementation.
- [ ] Target/OS/ABI/object/runtime decisions remain in the target layer when applicable.
- [ ] Backend changes remain limited to code generation, instruction selection, register allocation, or target queries when applicable.

<!-- Keep user-visible behavior documented when necessary. -->
## Documentation

<!-- Require documentation only when the public contract changed. -->
- [ ] Documentation/examples were updated when user-visible behavior changed, or no documentation change is required.
