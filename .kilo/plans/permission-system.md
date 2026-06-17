# Plan: Permission-Based Feature Gating for Veldanava

## Status

This document is an obsolete historical proposal. It described an earlier `manifest` / `grant` permission design that is not the active Veldanava syntax.

The active implementation now uses:

```veldanava
Primordial_Regalia;
```

plus:

- `genesis`
- `Incorporate`
- `Sanction`

Logical keywords `and`, `or`, and `not` are implemented for boolean-style expressions. `elif` is recognized but not implemented as a statement yet.

## Superseded proposal

The original proposal used:

```veldanava
manifest creative;
grant import;
```

and planned AST nodes like `PermissionDeclNode`, permission categories, and semantic validation passes.

That design was replaced by the current permission-keyword approach.

## Current implementation reference

See:

- `README.md`
- `docs/LANGUAGE_RULES.md`
- `.kilo/plans/keywords-permission-system.md`

## Current syntax summary

```veldanava
Primordial_Regalia;

genesis func main():
    genesis print("hello");
;

Sanction:
    plus;
    myfunc();
    myclass();
;
```

Sectioned sanction syntax is rejected:

```veldanava
Sanction:
    operators:
        plus;
    funcs:
        myfunc();
    oop:
        myclass();
;
```

## Remaining work from original idea

Some original goals still remain valid as future work:

- Semantic validation pass.
- Real module loading for `Incorporate`.
- Full `Sanction` backend policy beyond current semantic validation.
- Ownership/borrowing checks.
- Test migration to current syntax.
