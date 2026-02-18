# Security Policy

## Supported Versions

| Version | Supported          |
| ------- | ------------------ |
| 0.5.x   | :white_check_mark: |

## Reporting a Vulnerability

### How to Report

1. **Do NOT** create a public GitHub issue for security vulnerabilities
2. Email: info@radiantfoundation.org
3. Include:
   - Description of the vulnerability
   - Steps to reproduce
   - Potential impact

### Response Timeline

- **Initial Response:** Within 48 hours
- **Status Update:** Within 7 days

## Security Considerations

### Debugger Security

rxdeb is a script debugger and should be used in development environments only.

1. **Not for Production:** Never run the debugger on production systems
2. **Input Validation:** Be cautious with untrusted script input
3. **Network Access:** Electrum integration makes network requests

### Build Security

When building from source:

1. Verify you're building from the official repository
2. Check commit signatures if available
3. Use the provided build instructions

### Known Limitations

1. **Beta Status:** Software is in beta
2. **No Windows Binaries:** Limited Windows support
3. **Interpreter Differences:** May have subtle differences from node implementation

---

*Last updated: January 2026*
