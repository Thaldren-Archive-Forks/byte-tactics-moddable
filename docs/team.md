# How the lead and the maintainer work together

Two Claude sessions run the plan in #2:

- the **lead** (`/lead`) implements issues, opens pull requests and gets them to a mergeable state;
- the **maintainer** (`/maintainer`) reviews and merges pull requests, and keeps the issues true to the plan and to the work already merged.

They share one GitHub account, and they talk only through GitHub: pull request reviews, review comments, issue comments and the labels below. This file is the contract between them. Both roles also follow `AGENTS.md`.

## Signing

GitHub shows both roles as the same account, so every comment, review and reply starts with the role:

```
Lead: ...
Maintainer: ...
```

## Reviews

The account can't approve or request changes on its own pull requests, so the maintainer posts every review as a plain comment review whose first line is the **verdict**:

```
Maintainer review of <full head SHA>: approve
Maintainer review of <full head SHA>: changes
```

The rest of the body lists the findings, and specific findings go as inline comments on the lines they concern. A `changes` verdict lists each thing that must change; anything only suggested says "optional".

The latest verdict on a pull request, and its current head:

```sh
gh api repos/ozgb/byte-tactics-moddable/pulls/<PR>/reviews \
  --jq '[.[] | select(.body | startswith("Maintainer review of"))] | last | (.body // "no verdict") | split("\n")[0]'
gh api repos/ozgb/byte-tactics-moddable/pulls/<PR> --jq .head.sha
```

A pull request **needs review** when it is open, not a draft, and its head SHA differs from the SHA in its latest verdict (or it has none). Pushing new commits is how the author asks for another review: no label or mention is needed.

## Labels

| Label | Set by | Meaning | Cleared by |
|---|---|---|---|
| `needs-grooming` | the lead (or anyone) | An issue isn't concrete enough to implement, or is too big for one pull request. A comment says what's missing. | the maintainer, after fleshing out or splitting it |
| `needs-human` | either role | A decision only the human can make: a change to the plan or its decisions, a disagreement after one round, repository settings, releases, anything outward-facing. A comment states the question and the options. | the human |

Neither role works on an item labelled `needs-human`.

## Issue states

- **Available:** open, no assignee, no open blockers (`blocked_by` API, mirrored in the "Blocked by" line), not labelled `needs-grooming` or `needs-human`.
- **Claimed:** assigned, with a "Lead: Claimed" comment. One claim at a time per lead.
- **Done:** closed by the merged pull request's `Closes #N`.

Blockers are kept in two places that must agree: the `blocked_by` dependency API, which tools query, and the "**Blocked by:**" line at the top of the body, which people read. Whoever changes one changes the other.

## Disagreements

The lead may reply to a finding with reasons instead of a change. The maintainer then either accepts and withdraws it in the next review, or keeps it. If it is still contested after that second round, the maintainer labels the pull request `needs-human` and states both positions.
