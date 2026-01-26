# Contributing to rxdeb

Thank you for your interest in contributing to rxdeb! This document provides guidelines for contributing.

## Getting Started

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+)
- autoconf, automake, libtool
- Git with submodule support

### Building from Source

```bash
# Clone with submodules
git clone --recursive https://github.com/radiantblockchain/rxdeb.git
cd rxdeb

# Build with Radiant support
./autogen.sh
./configure --enable-rxd
make -j$(nproc)

# Run tests
./test-rxdeb
```

## Development Workflow

### Branch Naming

- `feature/` - New features
- `fix/` - Bug fixes
- `docs/` - Documentation updates
- `refactor/` - Code refactoring
- `test/` - Test additions/improvements

### Commit Messages

Follow conventional commits:

```
type(scope): description

[optional body]

[optional footer]
```

Types: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`

Examples:
```
feat(rxd): add OP_PUSHINPUTREF support
fix(electrum): handle connection timeout gracefully
docs(examples): add fungible token tutorial
```

### Code Style

- Follow existing code style
- Use 4 spaces for indentation (C++)
- Keep lines under 100 characters
- Use meaningful variable names
- Add comments for complex logic

### Testing

All new features should include tests:

```cpp
// tests/rxd/rxd_*_tests.cpp

TEST_CASE("OP_PUSHINPUTREF pushes correct reference", "[rxd][reference]") {
    // Test implementation
}
```

Run tests:
```bash
# All tests
./test-rxdeb

# Radiant-specific tests only
./test-rxdeb "[rxd]"

# Specific test category
./test-rxdeb "[rxd][reference]"
```

## Pull Request Process

1. **Fork** the repository
2. **Create** a feature branch
3. **Make** your changes
4. **Test** thoroughly
5. **Submit** a pull request

### PR Checklist

- [ ] Code compiles without warnings
- [ ] All tests pass
- [ ] New tests added for new functionality
- [ ] Documentation updated if needed
- [ ] Commit messages follow convention
- [ ] PR description explains changes

## Adding New Opcodes

When adding support for new Radiant opcodes:

1. Add opcode to `rxd/rxd_script.h` enum
2. Implement in `rxd/rxd_vm_adapter.cpp`
3. Add to `GetOpName()` in `rxd/rxd_script.cpp`
4. Add tests in `tests/rxd/rxd_script_tests.cpp`
5. Add test vectors to `tests/rxd/data/`
6. Update documentation

### Opcode Implementation Template

```cpp
// In rxd_vm_adapter.cpp
case OP_NEW_OPCODE: {
    // 1. Check stack requirements
    if (stack.size() < required_args)
        return set_error(serror, ScriptError::INVALID_STACK_OPERATION);
    
    // 2. Pop arguments
    valtype& arg = stacktop(-1);
    
    // 3. Perform operation
    // ...
    
    // 4. Push result
    stack.push_back(result);
    
    // 5. Pop consumed arguments
    popstack(stack);
    
    break;
}
```

## Electrum Integration

When working on Electrum features:

- Test against both mainnet and testnet
- Handle network errors gracefully
- Add appropriate timeouts
- Log connection issues for debugging

## Consensus Accuracy

**Critical**: rxdeb must produce identical results to Radiant-Core.

- Use the Radiant-Core submodule for consensus-critical code
- Generate golden tests from a running node
- Compare outputs for edge cases

```bash
# Compare with radiant-cli
./rxdeb --script="..." | diff - <(radiant-cli decodescript ...)
```

## Questions?

- Open an issue for questions
- Join the Radiant community Discord
- Review existing code and tests

## License

By contributing, you agree that your contributions will be licensed under the MIT License.
