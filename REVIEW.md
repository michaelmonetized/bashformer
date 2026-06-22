# FALLOW REVIEW

## HEALTH

## Vital Signs

| Metric | Value |
|:-------|------:|
| Total LOC | 371 |
| Avg Cyclomatic | 3.6 |
| P90 Cyclomatic | 8 |
| Dead Files | 50.0% |
| Dead Exports | 0.0% |
| Maintainability (avg) | 83.3 |
| Circular Deps | 0 |
| Unused Deps | 0 |

## Fallow: 5 high complexity functions

| File | Function | Severity | Cyclomatic | Cognitive | CRAP | Lines |
|:-----|:---------|:---------|:-----------|:----------|:-----|:------|
| `index.tsx:118` | `step` | critical | 21 **!** | 24 **!** | 462.0 **!** | 72 |
| `index.tsx:196` | `renderGame` | critical | 20 | 27 **!** | 420.0 **!** | 59 |
| `index.tsx:7` | `getTerminalSize` | high | 8 | 5 | 72.0 **!** | 13 |
| `index.tsx:81` | `handleInput` | high | 8 | 8 | 72.0 **!** | 26 |
| `__tests__/config.test.ts:5` | `getTerminalSize` | moderate | 5 | 2 | 30.0 **!** | 7 |

**2** files, **24** functions analyzed (thresholds: cyclomatic > 20, cognitive > 15, CRAP >= 30.0)



## AUDIT


Audit scope: 5 changed files vs master (6678fda..HEAD)
✓ No issues in 5 changed files (0.20s)


## DEAD

## Fallow: 1 issue found

### Unused files (1)

- `__tests__/config.test.ts`




## DUPLICATION

note: hid 1 clone group below minOccurrences=3 (lower --min-occurrences to see them)
## Fallow: no code duplication found



## DOCSTRINGS

✔︎ 100% docstring coverage

