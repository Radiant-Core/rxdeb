#include <cstdio>
#include <unistd.h>
#include <inttypes.h>

#include <instance.h>

#include <tinyformat.h>

#include <cliargs.h>

#include <datasets.h>

#include <functions.h>

#include <config/bitcoin-config.h>

#include <debugger/version.h>

#ifdef RXD_SUPPORT
#include <rxd/rxd_params.h>
#include <rxd/rxd_script.h>
#include <rxd/rxd_vm_adapter.h>
#include <rxd/rxd_electrum.h>
#include <rxd/rxd_repl.h>
#endif

bool quiet = false;
bool pipe_in = false;  // xxx | rxdeb
bool pipe_out = false; // rxdeb xxx > file
bool verbose = false;

#ifdef RXD_SUPPORT
// Radiant backend selection
enum class Backend { BITCOIN, RADIANT };
Backend active_backend = Backend::RADIANT;  // Default to Radiant
rxd::Network rxd_network = rxd::Network::MAINNET;
std::string electrum_server;
std::string artifact_file;
std::string artifact_function;
std::string artifact_args;
#endif

struct script_verify_flag {
    std::string str;
    uint32_t id;
    script_verify_flag(std::string str_in, uint32_t id_in) : str(str_in), id(id_in) {}
};

static const std::vector<script_verify_flag> svf {
    #define _(v) script_verify_flag(#v, SCRIPT_VERIFY_##v)
    _(P2SH),
    _(STRICTENC),
    _(DERSIG),
    _(LOW_S),
    _(NULLDUMMY),
    _(SIGPUSHONLY),
    _(MINIMALDATA),
    _(DISCOURAGE_UPGRADABLE_NOPS),
    _(CLEANSTACK),
    _(CHECKLOCKTIMEVERIFY),
    _(CHECKSEQUENCEVERIFY),
    _(WITNESS),
    _(DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM),
    _(MINIMALIF),
    _(NULLFAIL),
    _(WITNESS_PUBKEYTYPE),
    _(CONST_SCRIPTCODE),
    _(TAPROOT),
    _(DISCOURAGE_UPGRADABLE_TAPROOT_VERSION),
    _(DISCOURAGE_OP_SUCCESS),
    _(DISCOURAGE_UPGRADABLE_PUBKEYTYPE),
    #undef _
};

#ifdef RXD_SUPPORT
// Radiant-specific verification flags
static const std::vector<script_verify_flag> rxd_svf {
    #define _(v) script_verify_flag(#v, rxd::ScriptFlags::SCRIPT_##v)
    _(VERIFY_P2SH),
    _(VERIFY_STRICTENC),
    _(VERIFY_DERSIG),
    _(VERIFY_LOW_S),
    _(VERIFY_SIGPUSHONLY),
    _(VERIFY_MINIMALDATA),
    _(VERIFY_DISCOURAGE_UPGRADABLE_NOPS),
    _(VERIFY_CLEANSTACK),
    _(VERIFY_CHECKLOCKTIMEVERIFY),
    _(VERIFY_CHECKSEQUENCEVERIFY),
    _(VERIFY_MINIMALIF),
    _(VERIFY_NULLFAIL),
    _(ENABLE_SIGHASH_FORKID),
    _(64_BIT_INTEGERS),
    _(NATIVE_INTROSPECTION),
    _(ENHANCED_REFERENCES),
    _(PUSH_TX_STATE),
    #undef _
};
#endif

static const std::string svf_string(uint32_t flags, std::string separator = " ") {
    std::string s = "";
    while (flags) {
        for (const auto& i : svf) {
            if (flags & i.id) {
                flags ^= i.id;
                s += separator + i.str;
            }
        }
    }
    return s.size() ? s.substr(separator.size()) : "(none)";
}

static const unsigned int svf_get_flag(std::string s) {
    for (const auto& i : svf) if (i.str == s) return i.id;
    return 0;
}

static unsigned int svf_parse_flags(unsigned int in_flags, const char* mod) {
    char buf[128];
    bool adding;
    size_t j = 0;
    for (size_t i = 0; mod[i-(i>0)]; i++) {
        if (!mod[i] || mod[i] == ',') {
            buf[j] = 0;
            if (buf[0] != '+' && buf[0] != '-') {
                fprintf(stderr, "svf_parse_flags(): expected + or - near %s\n", buf);
                exit(1);
            }
            adding = buf[0] == '+';
            unsigned int f = svf_get_flag(&buf[1]);
            if (!f) {
                fprintf(stderr, "svf_parse_flags(): unknown verification flag: %s\n", &buf[1]);
                exit(1);
            }
            if (adding) in_flags |= f; else in_flags &= ~f;
            j = 0;
        } else buf[j++] = mod[i];
    }
    return in_flags;
}

void setup_debug_set(const std::string& debug_params, std::set<std::string>& debug_set)
{
    if (debug_set.empty() && !debug_params.empty()) delimiter_set(debug_params, debug_set);
}

bool get_debug_flag(const std::string& name, const std::set<std::string>& debug_set, bool fallback = false)
{
    // variant 1: debug parameter, lowercase, without prefix, contained in debug_set
    // variant 2: environment var, all caps, prefixed with 'DEBUG_'
    const auto& v = std::getenv(("DEBUG_" + ToUpper(name)).c_str());
    return debug_set.count(name) || (v ? strcmp("0", v) : fallback);
}

int main(int argc, char* const* argv)
{
    pipe_in = !isatty(fileno(stdin)) || std::getenv("DEBUG_SET_PIPE_IN");
    pipe_out = !isatty(fileno(stdout)) || std::getenv("DEBUG_SET_PIPE_OUT");
    if (pipe_in || pipe_out) btc_logf = btc_logf_dummy;

    cliargs ca;
    ca.add_option("help", 'h', no_arg);
    ca.add_option("quiet", 'q', no_arg);
    ca.add_option("tx", 'x', req_arg);
    ca.add_option("txin", 'i', req_arg);
    ca.add_option("modify-flags", 'f', req_arg);
    ca.add_option("select", 's', req_arg);
    ca.add_option("pretend-valid", 'P', req_arg);
    ca.add_option("default-flags", 'd', no_arg);
    ca.add_option("allow-disabled-opcodes", 'z', no_arg);
    ca.add_option("version", 'V', no_arg);
    ca.add_option("dataset", 'X', opt_arg);
    ca.add_option("verbose", 'v', no_arg);
    ca.add_option("debug", 'D', req_arg);
#ifdef RXD_SUPPORT
    // Radiant-specific options
    ca.add_option("btc", 'B', no_arg);           // Use Bitcoin backend
    ca.add_option("network", 'n', req_arg);      // mainnet/testnet/regtest
    ca.add_option("electrum", 'e', req_arg);     // Electrum server host:port
    ca.add_option("txid", 't', req_arg);         // Fetch tx by txid from Electrum
    ca.add_option("vin", 'I', req_arg);          // Input index when using --txid
    ca.add_option("artifact", 'a', req_arg);     // RadiantScript artifact JSON
    ca.add_option("function", 'F', req_arg);     // Contract function name
    ca.add_option("args", 'A', req_arg);         // Function arguments JSON
    ca.add_option("context", 'c', req_arg);      // Execution context JSON file
    ca.add_option("refs", 'r', no_arg);          // Show reference tracking
    ca.add_option("source", 'S', no_arg);        // Show RadiantScript source
#endif
    ca.parse(argc, argv);
    quiet = ca.m.count('q') || pipe_in || pipe_out;

    btcdeb_verbose = verbose = ca.m.count('v');
    if (quiet && verbose) {
        fprintf(stderr, "You cannot both require silence and verbosity.\n");
        exit(1);
    }

#ifdef RXD_SUPPORT
    // Process Radiant-specific options first
    if (ca.m.count('B')) {
        active_backend = Backend::BITCOIN;
    }
    if (ca.m.count('n')) {
        std::string net = ca.m['n'];
        try {
            rxd_network = rxd::ParseNetwork(net);
        } catch (const std::exception& e) {
            fprintf(stderr, "Invalid network: %s (use mainnet, testnet, or regtest)\n", net.c_str());
            return 1;
        }
    }
    if (ca.m.count('e')) {
        electrum_server = ca.m['e'];
    }
    if (ca.m.count('a')) {
        artifact_file = ca.m['a'];
    }
    if (ca.m.count('F')) {
        artifact_function = ca.m['F'];
    }
    if (ca.m.count('A')) {
        artifact_args = ca.m['A'];
    }
#endif

    if (ca.m.count('h')) {
        fprintf(stderr, "rxdeb - Radiant Script Debugger v" VERSION() "\n\n");
        fprintf(stderr, "Usage: %s [OPTIONS] [<script> [<stack args>...]]\n\n", argv[0]);
        fprintf(stderr, "RADIANT OPTIONS:\n");
        fprintf(stderr, "  --btc|-B                 Use Bitcoin backend instead of Radiant\n");
        fprintf(stderr, "  --network|-n <net>       Network: mainnet, testnet, regtest (default: mainnet)\n");
        fprintf(stderr, "  --electrum|-e <host:port> Electrum server for fetching UTXOs\n");
        fprintf(stderr, "  --txid|-t <txid>         Fetch transaction by txid from Electrum\n");
        fprintf(stderr, "  --vin|-I <index>         Input index to debug (with --txid)\n");
        fprintf(stderr, "  --artifact|-a <file>     RadiantScript artifact JSON file\n");
        fprintf(stderr, "  --function|-F <name>     Contract function to debug\n");
        fprintf(stderr, "  --args|-A <json>         Function arguments as JSON array\n");
        fprintf(stderr, "  --context|-c <file>      Execution context JSON file\n");
        fprintf(stderr, "  --refs|-r                Show reference tracking state\n");
        fprintf(stderr, "  --source|-S              Show RadiantScript source (if available)\n\n");
        fprintf(stderr, "GENERAL OPTIONS:\n");
        fprintf(stderr, "  --tx|-x <hex>            Spending transaction hex\n");
        fprintf(stderr, "  --txin|-i <hex>          Input transaction hex\n");
        fprintf(stderr, "  --select|-s <index>      Select input index\n");
        fprintf(stderr, "  --modify-flags|-f <flags> Modify verification flags (+FLAG,-FLAG)\n");
        fprintf(stderr, "  --default-flags|-d       Show default verification flags\n");
        fprintf(stderr, "  --verbose|-v             Verbose output\n");
        fprintf(stderr, "  --quiet|-q               Quiet mode\n");
        fprintf(stderr, "  --version|-V             Show version\n");
        fprintf(stderr, "  --help|-h                Show this help\n\n");
        fprintf(stderr, "EXAMPLES:\n");
        fprintf(stderr, "  # Debug a simple script\n");
        fprintf(stderr, "  %s '[OP_1 OP_2 OP_ADD OP_3 OP_EQUAL]'\n\n", argv[0]);
        fprintf(stderr, "  # Debug with Electrum (fetch live UTXO)\n");
        fprintf(stderr, "  %s --electrum=electrum.radiant.ovh:50002 --txid=<txid> --vin=0\n\n", argv[0]);
        fprintf(stderr, "  # Debug RadiantScript contract\n");
        fprintf(stderr, "  %s --artifact=Token.json --function=transfer --tx=<hex>\n\n", argv[0]);
        fprintf(stderr, "For Bitcoin compatibility, use --btc flag.\n");
        return 0;
    } else if (ca.m.count('d')) {
#ifdef RXD_SUPPORT
        if (active_backend == Backend::RADIANT) {
            printf("Radiant standard verification flags:\n・ %s\n", 
                   svf_string(rxd::ScriptFlags::STANDARD_SCRIPT_VERIFY_FLAGS, "\n・ ").c_str());
        } else {
            printf("Bitcoin standard verification flags:\n・ %s\n", 
                   svf_string(STANDARD_SCRIPT_VERIFY_FLAGS, "\n・ ").c_str());
        }
#else
        printf("The standard (enabled by default) flags are:\n・ %s\n", svf_string(STANDARD_SCRIPT_VERIFY_FLAGS, "\n・ ").c_str());
#endif
        return 0;
    } else if (ca.m.count('V')) {
#ifdef RXD_SUPPORT
        printf("rxdeb (\"Radiant Script Debugger\") v" VERSION() "\n");
        printf("Backend: %s\n", active_backend == Backend::RADIANT ? "Radiant" : "Bitcoin");
        if (active_backend == Backend::RADIANT) {
            printf("Network: %s\n", rxd::NetworkName(rxd_network).c_str());
        }
#else
        printf("rxdeb (\"Script Debugger\") v" VERSION() "\n");
#endif
        return 0;
    } else if (ca.m.count('X')) {
        process_datasets(ca.m, verbose);
    } else if (!quiet) {
#ifdef RXD_SUPPORT
        printf("rxdeb v" VERSION() " [%s] -- type `%s -h` for options\n", 
               active_backend == Backend::RADIANT ? "Radiant" : "Bitcoin", argv[0]);
#else
        printf("rxdeb v" VERSION() " -- type `%s -h` for options\n", argv[0]);
#endif
    }

    if (!pipe_in) {
        std::set<std::string> debug_set;
        setup_debug_set(ca.m['D'], debug_set);
        // temporarily defaulting most to ON
        if (get_debug_flag("sighash", debug_set)) btc_sighash_logf = btc_logf_stderr;
        if (get_debug_flag("signing", debug_set, true)) btc_sign_logf = btc_logf_stderr;
        // Note: segwit and taproot are NOT supported in Radiant
        btc_logf("LOG:");
        if (btc_enabled(btc_sighash_logf)) btc_logf(" sighash");
        if (btc_enabled(btc_sign_logf)) btc_logf(" signing");
        btc_logf("\n");
        btc_logf("notice: rxdeb has gotten quieter; use --verbose if necessary (this message is temporary)\n");
    }

    unsigned int flags = STANDARD_SCRIPT_VERIFY_FLAGS;
    if (ca.m.count('f')) {
        flags = svf_parse_flags(flags, ca.m['f'].c_str());
        if (verbose) fprintf(stderr, "resulting flags:\n・ %s\n", svf_string(flags, "\n・ ").c_str());
    }
    bool allow_disabled_opcodes = ca.m.count('z') > 0;

    int selected = -1;
    if (ca.m.count('s')) {
        selected = atoi(ca.m['s'].c_str());
    }

    if (ca.l.size() > 0 && !strncmp(ca.l[0], "tx=", 3)) {
        // backwards compatibility; move into tx
        ca.m['x'] = &ca.l[0][3];
        ca.l.erase(ca.l.begin(), ca.l.begin() + 1);
    }

    // crude check for tx=
    if (ca.m.count('x')) {
        try {
            if (!instance.parse_transaction(ca.m['x'].c_str(), true)) {
                return 1;
            }
        } catch (std::exception const& ex) {
            fprintf(stderr, "error parsing spending (--tx) transaction: %s\n", ex.what());
            return 1;
        }
        if (verbose) fprintf(stderr, "got %stransaction %s:\n%s\n", instance.sigver == SigVersion::WITNESS_V0 ? "segwit " : "", instance.tx->GetHash().ToString().c_str(), instance.tx->ToString().c_str());
    }
    if (ca.m.count('i')) {
        try {
            if (!instance.parse_input_transaction(ca.m['i'].c_str(), selected)) {
                return 1;
            }
        } catch (std::exception const& ex) {
            fprintf(stderr, "error parsing input (--txin) transaction: %s\n", ex.what());
            return 1;
        }
        if (verbose) fprintf(stderr, "got input tx #%" PRId64 " %s:\n%s\n", instance.txin_index, instance.txin->GetHash().ToString().c_str(), instance.txin->ToString().c_str());
    }
    char* script_str = nullptr;
    if (pipe_in) {
        char buf[1024];
        if (!fgets(buf, 1024, stdin)) {
            fprintf(stderr, "warning: no input\n");
        }
        int len = strlen(buf);
        while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r')) buf[--len] = 0;
        script_str = strdup(buf);
    } else if (ca.l.size() > 0) {
        script_str = strdup(ca.l[0]);
        ca.l.erase(ca.l.begin(), ca.l.begin() + 1);
    }

    if (ca.m.count('P')) {
        if (!instance.parse_pretend_valid_expr(ca.m['P'].c_str())) {
            return 1;
        }
    }

    CScript script;
    if (script_str) {
        if (instance.parse_script(script_str)) {
            if (verbose) btc_logf("valid script\n");
        } else {
            fprintf(stderr, "invalid script\n");
            return 1;
        }
        free(script_str);
    }

    instance.parse_stack_args(ca.l);

    if (instance.txin && instance.tx && ca.l.size() == 0 && instance.script.size() == 0) {
        if (!instance.configure_tx_txin()) return 1;
    }

    if (!instance.setup_environment(flags)) {
        fprintf(stderr, "failed to initialize script environment: %s\n", instance.error_string().c_str());
        return 1;
    }

    env = instance.env;
    env->allow_disabled_opcodes = allow_disabled_opcodes;

    std::vector<CScript*> script_ptrs;
    std::vector<std::string> script_headers;

    script_ptrs.push_back(&env->script);
    script_headers.push_back("");
    CScript::const_iterator it = env->script.begin();
    opcodetype opcode;
    valtype vchPushValue, p2sh_script_payload;
    while (env->script.GetOp(it, opcode, vchPushValue)) { p2sh_script_payload = vchPushValue; ++count; }

    std::vector<std::string> tc_desc;
    CScript p2sh_script;
    bool has_p2sh = false;
    if (env->is_p2sh && env->p2shstack.size() > 0) {
        has_p2sh = true;
        const valtype& p2sh_script_val = env->p2shstack.back();
        p2sh_script = CScript(p2sh_script_val.begin(), p2sh_script_val.end());
    } else if (env->sigversion == SigVersion::TAPSCRIPT) {
        // add commitment phase
        tc_desc = env->tce->Description();
        count += tc_desc.size();
    }
    if (instance.successor_script.size()) {
        script_ptrs.push_back(&instance.successor_script);
        script_headers.push_back("<<< scriptPubKey >>>");
        count++;
        it = instance.successor_script.begin();
        while (instance.successor_script.GetOp(it, opcode, vchPushValue)) ++count;
        if ((env->flags & SCRIPT_VERIFY_P2SH) && instance.successor_script.IsPayToScriptHash()) {
            has_p2sh = true;
            p2sh_script = CScript(p2sh_script_payload.begin(), p2sh_script_payload.end());
        }
    }
    if (has_p2sh) {
        script_ptrs.push_back(&p2sh_script);
        script_headers.push_back("<<< P2SH script >>>");
        count++;
        it = p2sh_script.begin();
        while (p2sh_script.GetOp(it, opcode, vchPushValue)) ++count;
    }
    script_lines = (char**)malloc(sizeof(char*) * count);

    int i = 0;
    char buf[1024];
    if (env->sigversion == SigVersion::TAPSCRIPT) {
        for (const auto& s : tc_desc) {
            script_lines[i++] = strdup(strprintf("#%04d %s", i, s).c_str());
        }
    }
    for (size_t siter = 0; siter < script_ptrs.size(); ++siter) {
        CScript* script = script_ptrs[siter];
        const std::string& header = script_headers[siter];
        if (header != "") script_lines[i++] = strdup(header.c_str());
        it = script->begin();
        while (script->GetOp(it, opcode, vchPushValue)) {
            char* pbuf = buf;
            pbuf += snprintf(pbuf, 1024, "#%04d ", i);
            if (vchPushValue.size() > 0) {
                snprintf(pbuf, 1024 + pbuf - buf, "%s", HexStr(std::vector<uint8_t>(vchPushValue.begin(), vchPushValue.end())).c_str());
            } else {
                snprintf(pbuf, 1024 + pbuf - buf, "%s", GetOpName(opcode).c_str());
            }
            script_lines[i++] = strdup(buf);
        }
    }

    if (instance.has_preamble) {
        if (verbose) btc_logf(
            "*** note: there is a for-clarity preamble\n\n"

            "This is a virtual script that rxdeb generates and presents to you so you can step through the validation process one step at a time. The input is simply the redeem script hash, whereas rxdeb presents it as a OP_DUP, OP_HASH160, <that hash>, OP_EQUALVERIFY script.\n"
        ); else if (!quiet) btc_logf("note: there is a for-clarity preamble (use --verbose for details)\n");
    }

    if (pipe_in || pipe_out) {
        if (!ContinueScript(*env)) {
            fprintf(stderr, "error: %s\n", ScriptErrorString(*env->serror).c_str());
            print_dualstack();
            return 1;
        }

        print_stack(env->stack, true);
        return 0;
    } else {
        kerl_set_history_file(".rxdeb_history");
        kerl_set_repeat_on_empty(true);
        kerl_set_enable_sensitivity();
        kerl_set_comment_char('#');
        kerl_register("step", fn_step, "Execute one instruction and iterate in the script.");
        kerl_register("rewind", fn_rewind, "Go back in time one instruction.");
        kerl_register("stack", fn_stack, "Print stack content.");
        kerl_register("altstack", fn_altstack, "Print altstack content.");
        kerl_register("vfexec", fn_vfexec, "Print vfexec content.");
        kerl_register("exec", fn_exec, "Execute command.");
        kerl_register("tf", fn_tf, "Transform a value using a given function.");
        kerl_set_completor("exec", compl_exec, true);
        kerl_set_completor("tf", compl_tf, false);
        kerl_register("print", fn_print, "Print script.");
#ifdef RXD_SUPPORT
        // Radiant-specific REPL commands
        kerl_register("refs", fn_refs, "Print reference tracking state (Radiant).");
        kerl_register("context", fn_context, "Print execution context (Radiant).");
        kerl_register("source", fn_source, "Print RadiantScript source location.");
#endif
        kerl_register_help("help");
        if (!quiet) btc_logf("%d op script loaded. type `help` for usage information\n", count);
        print_dualstack();
        if (env->curr_op_seq < count) {
            printf("%s\n", script_lines[env->curr_op_seq]);
        }
#ifdef RXD_SUPPORT
        kerl_run(active_backend == Backend::RADIANT ? "rxdeb> " : "btcdeb> ");
#else
        kerl_run("rxdeb> ");  
#endif
    }
}

static const char* opnames[] = {
    // push value
    "OP_0",
    "OP_FALSE",
    "OP_PUSHDATA1",
    "OP_PUSHDATA2",
    "OP_PUSHDATA4",
    "OP_1NEGATE",
    "OP_RESERVED",
    "OP_1",
    "OP_TRUE",
    "OP_2",
    "OP_3",
    "OP_4",
    "OP_5",
    "OP_6",
    "OP_7",
    "OP_8",
    "OP_9",
    "OP_10",
    "OP_11",
    "OP_12",
    "OP_13",
    "OP_14",
    "OP_15",
    "OP_16",

    // control
    "OP_NOP",
    "OP_VER",
    "OP_IF",
    "OP_NOTIF",
    "OP_VERIF",
    "OP_VERNOTIF",
    "OP_ELSE",
    "OP_ENDIF",
    "OP_VERIFY",
    "OP_RETURN",

    // stack ops
    "OP_TOALTSTACK",
    "OP_FROMALTSTACK",
    "OP_2DROP",
    "OP_2DUP",
    "OP_3DUP",
    "OP_2OVER",
    "OP_2ROT",
    "OP_2SWAP",
    "OP_IFDUP",
    "OP_DEPTH",
    "OP_DROP",
    "OP_DUP",
    "OP_NIP",
    "OP_OVER",
    "OP_PICK",
    "OP_ROLL",
    "OP_ROT",
    "OP_SWAP",
    "OP_TUCK",

    // splice ops
    "OP_CAT",
    "OP_SUBSTR",
    "OP_LEFT",
    "OP_RIGHT",
    "OP_SIZE",

    // bit logic
    "OP_INVERT",
    "OP_AND",
    "OP_OR",
    "OP_XOR",
    "OP_EQUAL",
    "OP_EQUALVERIFY",
    "OP_RESERVED1",
    "OP_RESERVED2",

    // numeric
    "OP_1ADD",
    "OP_1SUB",
    "OP_2MUL",
    "OP_2DIV",
    "OP_NEGATE",
    "OP_ABS",
    "OP_NOT",
    "OP_0NOTEQUAL",

    "OP_ADD",
    "OP_SUB",
    "OP_MUL",
    "OP_DIV",
    "OP_MOD",
    "OP_LSHIFT",
    "OP_RSHIFT",

    "OP_BOOLAND",
    "OP_BOOLOR",
    "OP_NUMEQUAL",
    "OP_NULEQUALVERIFY",
    "OP_NUMNOTEQUAL",
    "OP_LESSTHAN",
    "OP_GREATERTHAN",
    "OP_LESSTHANOREQUAL",
    "OP_GREATERTHANOREQUAL",
    "OP_MIN",
    "OP_MAX",

    "OP_WITHIN",

    // crypto
    "OP_RIPEMD160",
    "OP_SHA1",
    "OP_SHA256",
    "OP_HASH160",
    "OP_HASH256",
    "OP_CODESEPARATOR",
    "OP_CHECKSIG",
    "OP_CHECKSIGVERIFY",
    "OP_CHECKMULTISIG",
    "OP_CHECKMULTISIGVERIFY",
    "OP_CHECKSIGADD",

    // expansion
    "OP_NOP1",
    "OP_CHECKLOCKTIMEVERIFY",
    "OP_NOP2",
    "OP_CHECKSEQUENCEVERIFY",
    "OP_NOP3",
    "OP_NOP4",
    "OP_NOP5",
    "OP_NOP6",
    "OP_NOP7",
    "OP_NOP8",
    "OP_NOP9",
    "OP_NOP10",

#ifdef RXD_SUPPORT
    // Radiant: BCH-derived opcodes
    "OP_CHECKDATASIG",
    "OP_CHECKDATASIGVERIFY",
    "OP_REVERSEBYTES",

    // Radiant: State separator
    "OP_STATESEPARATOR",
    "OP_STATESEPARATORINDEX_UTXO",
    "OP_STATESEPARATORINDEX_OUTPUT",

    // Radiant: Native introspection
    "OP_INPUTINDEX",
    "OP_ACTIVEBYTECODE",
    "OP_TXVERSION",
    "OP_TXINPUTCOUNT",
    "OP_TXOUTPUTCOUNT",
    "OP_TXLOCKTIME",
    "OP_UTXOVALUE",
    "OP_UTXOBYTECODE",
    "OP_OUTPOINTTXHASH",
    "OP_OUTPOINTINDEX",
    "OP_INPUTBYTECODE",
    "OP_INPUTSEQUENCENUMBER",
    "OP_OUTPUTVALUE",
    "OP_OUTPUTBYTECODE",

    // Radiant: SHA512/256
    "OP_SHA512_256",
    "OP_HASH512_256",

    // Radiant: Reference opcodes
    "OP_PUSHINPUTREF",
    "OP_REQUIREINPUTREF",
    "OP_DISALLOWPUSHINPUTREF",
    "OP_DISALLOWPUSHINPUTREFSIBLING",
    "OP_REFHASHDATASUMMARY_UTXO",
    "OP_REFHASHVALUESUM_UTXOS",
    "OP_REFHASHDATASUMMARY_OUTPUT",
    "OP_REFHASHVALUESUM_OUTPUTS",
    "OP_PUSHINPUTREFSINGLETON",
    "OP_REFTYPE_UTXO",
    "OP_REFTYPE_OUTPUT",
    "OP_REFVALUESUM_UTXOS",
    "OP_REFVALUESUM_OUTPUTS",
    "OP_REFOUTPUTCOUNT_UTXOS",
    "OP_REFOUTPUTCOUNT_OUTPUTS",
    "OP_REFOUTPUTCOUNTZEROVALUED_UTXOS",
    "OP_REFOUTPUTCOUNTZEROVALUED_OUTPUTS",
    "OP_REFDATASUMMARY_UTXO",
    "OP_REFDATASUMMARY_OUTPUT",
    "OP_CODESCRIPTHASHVALUESUM_UTXOS",
    "OP_CODESCRIPTHASHVALUESUM_OUTPUTS",
    "OP_CODESCRIPTHASHOUTPUTCOUNT_UTXOS",
    "OP_CODESCRIPTHASHOUTPUTCOUNT_OUTPUTS",
    "OP_CODESCRIPTHASHZEROVALUEDOUTPUTCOUNT_UTXOS",
    "OP_CODESCRIPTHASHZEROVALUEDOUTPUTCOUNT_OUTPUTS",
    "OP_CODESCRIPTBYTECODE_UTXO",
    "OP_CODESCRIPTBYTECODE_OUTPUT",
    "OP_STATESCRIPTBYTECODE_UTXO",
    "OP_STATESCRIPTBYTECODE_OUTPUT",
    "OP_PUSH_TX_STATE",
#endif

    nullptr,
};

char* compl_exec(const char* text, int continued) {
    static int list_index, len;
	const char *name;

	/* If this is a new word to complete, initialize now.  This includes
	 saving the length of TEXT for efficiency, and initializing the index
	 variable to 0. */
	if (!continued) {
		list_index = -1;
		len = strlen(text);
	}

	/* Return the next name which partially matches from the names list. */
	while (opnames[++list_index]) {
		name = opnames[list_index];

		if (strncasecmp(name, text, len) == 0)
			return strdup(name);
	}

	/* If no names matched, then return NULL. */
	return (char *)NULL;
}
