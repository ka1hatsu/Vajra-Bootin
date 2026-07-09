#include "recommender/Recommender.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

using vajra::catalog::Distro;
using vajra::hardware::HardwareProfile;

int failures = 0;

void check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

HardwareProfile hardware(std::string architecture = "x86_64", double ram_gib = 8.0,
                         unsigned processors = 4, std::string firmware = "UEFI") {
    HardwareProfile profile{};
    profile.architecture = std::move(architecture);
    profile.memory_bytes = static_cast<std::uint64_t>(ram_gib * 1024.0 * 1024.0 * 1024.0);
    profile.logical_processors = processors;
    profile.firmware_mode = std::move(firmware);
    return profile;
}

Distro distro(std::string id = "test", double minimum_ram = 2.0,
              double recommended_ram = 4.0, unsigned minimum_processors = 2) {
    return {std::move(id), "Test Linux", minimum_ram, recommended_ram,
            minimum_processors, {"x86_64"}, "Xfce", "beginner",
            {"daily_use"}, "https://example.invalid/", true, true};
}

void test_rejects_wrong_architecture() {
    auto candidate = distro();
    candidate.architectures = {"arm64"};
    const std::vector<Distro> candidates{candidate};
    const auto results = vajra::recommender::recommend(hardware(), candidates, {});
    check(results.empty(), "incompatible architecture must be filtered out");
}

void test_accepts_amd64_alias() {
    auto candidate = distro();
    candidate.architectures = {"amd64"};
    const std::vector<Distro> candidates{candidate};
    const auto results = vajra::recommender::recommend(hardware("x86_64"), candidates, {});
    check(results.size() == 1, "x86_64 hardware must accept amd64 catalog alias");
}

void test_rejects_insufficient_memory() {
    const std::vector<Distro> candidates{distro()};
    const auto results = vajra::recommender::recommend(hardware("x86_64", 1.0), candidates, {});
    check(results.empty(), "distribution above available memory must be filtered out");
}

void test_rejects_insufficient_processors() {
    const std::vector<Distro> candidates{distro()};
    const auto results = vajra::recommender::recommend(hardware("x86_64", 8.0, 1), candidates, {});
    check(results.empty(), "distribution above processor requirement must be filtered out");
}

void test_rejects_firmware_mismatch() {
    auto candidate = distro();
    candidate.supports_uefi = false;
    candidate.supports_legacy_bios = true;
    const std::vector<Distro> candidates{candidate};
    const auto results = vajra::recommender::recommend(hardware("x86_64", 8.0, 4, "UEFI"), candidates, {});
    check(results.empty(), "UEFI machine must reject a legacy-only distribution");
}

void test_preference_match_changes_ranking() {
    auto daily = distro("daily");
    auto coding = distro("coding");
    coding.categories = {"coding"};
    const std::vector<Distro> candidates{daily, coding};
    const auto results = vajra::recommender::recommend(hardware(), candidates, {"coding", "beginner"});
    check(results.size() == 2, "both compatible distributions should remain");
    check(results.size() < 2 || results.front().distro->id == "coding", "purpose match should rank first");
}

void test_scores_are_capped() {
    auto candidate = distro();
    candidate.categories = {"daily_use", "old_pc"};
    const std::vector<Distro> candidates{candidate};
    const auto results = vajra::recommender::recommend(hardware("x86_64", 4.0), candidates, {"daily_use", "beginner"});
    check(results.size() == 1, "compatible candidate should be returned");
    check(results.empty() || results.front().score <= 100, "score must never exceed 100");
}

void test_results_are_sorted_descending() {
    auto preferred = distro("preferred");
    preferred.categories = {"coding"};
    auto ordinary = distro("ordinary");
    ordinary.difficulty = "intermediate";
    ordinary.categories = {"stable"};
    const std::vector<Distro> candidates{ordinary, preferred};
    const auto results = vajra::recommender::recommend(hardware(), candidates, {"coding", "beginner"});
    check(results.size() == 2, "sorting test needs two compatible results");
    check(results.size() < 2 || results[0].score >= results[1].score, "results must be sorted by descending score");
}

} // namespace

int main() {
    test_rejects_wrong_architecture();
    test_accepts_amd64_alias();
    test_rejects_insufficient_memory();
    test_rejects_insufficient_processors();
    test_rejects_firmware_mismatch();
    test_preference_match_changes_ranking();
    test_scores_are_capped();
    test_results_are_sorted_descending();

    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All recommender tests passed\n";
    return EXIT_SUCCESS;
}
