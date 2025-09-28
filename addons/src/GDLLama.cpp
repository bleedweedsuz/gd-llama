#include "GDLLama.hpp"
#include "llama.h"

#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <vector>
#include <algorithm>
#include <thread>

using namespace godot;

void GDLLama::_bind_methods() {
    ClassDB::bind_method(D_METHOD("ask", "prompt"), &GDLLama::ask);
    ClassDB::bind_method(D_METHOD("load_model", "path"), &GDLLama::load_model);
    ClassDB::bind_method(D_METHOD("cleanup"), &GDLLama::cleanup);

    ClassDB::bind_method(D_METHOD("set_context_size", "size"), &GDLLama::set_context_size);
    ClassDB::bind_method(D_METHOD("get_context_size"), &GDLLama::get_context_size);

    ClassDB::bind_method(D_METHOD("kv_cache_info"), &GDLLama::kv_cache_info);
    ClassDB::bind_method(D_METHOD("reset_context"), &GDLLama::reset_context);
    ClassDB::bind_method(D_METHOD("system_info"), &GDLLama::system_info);
}

GDLLama::GDLLama() {
    UtilityFunctions::print("GDLLama initialized - Version 0.0.2");
    UtilityFunctions::print(system_info());
}

GDLLama::~GDLLama() {
    cleanup();
    UtilityFunctions::print("GDLLama destroyed!");
}

void GDLLama::set_context_size(int size) {
    context_size = size;
}

int GDLLama::get_context_size() {
    return context_size;
}

void GDLLama::load_model(String path) {
    if (path.is_empty()) { return;}

    cleanup();
    String abs_path = godot::ProjectSettings::get_singleton()->globalize_path(path);
    std::string model_path = abs_path.utf8().get_data();
    try{
        // Load the model
        llama_model_params model_params = llama_model_default_params();
        model = llama_model_load_from_file(model_path.c_str(), model_params);

        if (!model) {
            UtilityFunctions::print("Error: Failed to load model at: " + path);
            return;
        }

        llama_context_params ctx_params = llama_context_default_params();
        ctx_params.n_ctx = context_size;

        ctx_params.n_threads = std::max(1, (int)std::thread::hardware_concurrency() - 1);
        ctx_params.n_threads_batch = ctx_params.n_threads;

        context = llama_init_from_model(model, ctx_params);

        if (!context) {
            UtilityFunctions::print("Error: Failed to create context!");
            cleanup();
            return;
        }

        UtilityFunctions::print("Model loaded successfully from: " + path);
    }
    catch(...){
        UtilityFunctions::print("Exception in model path: " + path);
    }
}

String GDLLama::ask(String prompt) {
    if (!context || !model) return "Error: No model loaded!";

    std::string utf8_prompt = std::string(prompt.utf8().get_data());

    const llama_vocab * vocab = llama_model_get_vocab(model);
    if (!vocab) return "Error: Failed to get vocab!";

    // Tokenize with BOS + specials (matches llama.cpp examples)
    const int n_prompt = -llama_tokenize(vocab, utf8_prompt.c_str(), utf8_prompt.size(), nullptr, 0, true, true);
    if (n_prompt < 0) return "Error: Failed to count tokens!";

    std::vector<llama_token> tokens(n_prompt);
    if (llama_tokenize(vocab, utf8_prompt.c_str(), utf8_prompt.size(), tokens.data(), tokens.size(), true, true) < 0) {
        return "Error: Failed to tokenize prompt!";
    }

    // Initialize batch from the prompt (no manual filling)
    llama_batch batch = llama_batch_get_one(tokens.data(), tokens.size());

    // Prime the model
    if (llama_decode(context, batch)) {
        return "Error: Failed initial decode!";
    }

    auto sparams = llama_sampler_chain_default_params();
    llama_sampler * smpl = llama_sampler_chain_init(sparams);
    llama_sampler_chain_add(smpl, llama_sampler_init_top_k(40)); // restrict to top 40
    llama_sampler_chain_add(smpl, llama_sampler_init_top_p(0.9f, 1)); // nucleus sampling
    llama_sampler_chain_add(smpl, llama_sampler_init_temp(0.8f)); // soften logits
    llama_sampler_chain_add(smpl, llama_sampler_init_penalties(64, 1.1f, 0.0f, 0.0f)); // repetition penalty
    llama_sampler_chain_add(smpl, llama_sampler_init_dist(LLAMA_DEFAULT_SEED)); // final stochastic pick

    std::string output; output.reserve(1024);
    int max_new_tokens = 64;

    for (int t = 0; t < max_new_tokens; t++) {
        llama_token new_tok = llama_sampler_sample(smpl, context, -1);
        if (llama_vocab_is_eog(vocab, new_tok)) {
            UtilityFunctions::print("EOS at step " + String::num(t));
            break;
        }

        char buf[256];
        int n = llama_token_to_piece(vocab, new_tok, buf, sizeof(buf), 0, true);
        if (n > 0) output.append(buf, n);

        // Feed the new token back
        batch = llama_batch_get_one(&new_tok, 1);
        if (llama_decode(context, batch)) break;
    }
    llama_sampler_free(smpl);
    return output.c_str();
}

String GDLLama::system_info() {
    const char *info = llama_print_system_info();
    return String("Llama.cpp system info:\n") + String(info);
}

void GDLLama::cleanup() {
    if (context) {
        llama_free(context); 
        context = nullptr;
    }
    if (model) {
        llama_model_free(model);
        model = nullptr;
    }
}

String GDLLama::kv_cache_info() {
    if (!context) return "Error: No context available.";
    
    std::stringstream ss;
    ss << "Context window tokens (n_ctx): " << llama_n_ctx(context) << "\n";
    ss << "State size (bytes): " << llama_state_get_size(context) << "\n";

    auto perf = llama_perf_context(context);
    ss << "Prompt tokens processed: " << perf.n_p_eval << "\n";
    ss << "Generated tokens: " << perf.n_eval << "\n";
    ss << "Reused KV slots: " << perf.n_reused << "\n";

    return String(ss.str().c_str());
}

void GDLLama::reset_context() {
    if (!model) return;
    if (context) llama_free(context);

    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = context_size;
    context = llama_init_from_model(model, ctx_params);
    UtilityFunctions::print("Context reset (new KV cache).");
}
