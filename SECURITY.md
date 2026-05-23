# Security Policy

## Reporting a vulnerability

Please do not open a public GitHub issue for suspected security vulnerabilities.

Instead, use a private channel:

- If GitHub Security Advisories are enabled for this repository, prefer "Report a vulnerability" from the Security tab.
- Otherwise, contact the maintainers privately (for example via email or another agreed private channel).

Include:

- A clear description of the issue and impact.
- A minimal reproduction (code and inputs).
- Affected versions / commit.

## Scope notes

`loxperm` is a header-only embedded C library. It performs no network I/O and does not include a general-purpose parser.
Vulnerability reports are still accepted, especially for issues that can lead to memory corruption, undefined behavior,
or incorrect control decisions in embedded applications.

