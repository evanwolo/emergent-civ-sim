#include <gtest/gtest.h>
#include "modules/Economy.h"
#include "kernel/Kernel.h"  // For Agent struct
#include <random>

// Basic economy initialization test
TEST(EconomyTest, Initialization) {
    std::mt19937_64 rng(42);
    Economy economy;
    economy.init(200, 10000, rng, "baseline");

    // Check basic properties - getRegion exists, check region 0
    const auto& region0 = economy.getRegion(0);
    EXPECT_GE(region0.development, 0.0);
    EXPECT_LE(region0.development, 1.0);
}

// Price bounds test - prices should be positive and not extreme
TEST(EconomyTest, PriceBounds) {
    std::mt19937_64 rng(42);
    Economy economy;
    economy.init(10, 500, rng, "baseline");

    // Create dummy agents for update
    std::vector<Agent> agents(500);
    for (std::size_t i = 0; i < agents.size(); ++i) {
        agents[i].region = i % 10;
        agents[i].alive = true;
    }

    // Run several updates
    std::vector<std::uint32_t> regionPopulations(10, 50);
    std::vector<std::array<double, 4>> regionBeliefs(10, {0, 0, 0, 0});

    for (int i = 0; i < 100; ++i) {
        economy.update(regionPopulations, regionBeliefs, agents, i, nullptr);
    }

    // Check that prices are positive and reasonably bounded
    // (wider bounds since economy model allows more variation)
    for (std::uint32_t r = 0; r < 10; ++r) {
        const auto& region = economy.getRegion(r);
        for (int g = 0; g < 5; ++g) {
            EXPECT_GT(region.prices[g], 0.0) << "Price should be positive";
            EXPECT_LT(region.prices[g], 1000.0) << "Price extremely high";
        }
    }
}

// Welfare computation test
TEST(EconomyTest, WelfareComputation) {
    std::mt19937_64 rng(42);
    Economy economy;
    economy.init(5, 100, rng, "baseline");

    // Create dummy agents
    std::vector<Agent> agents(100);
    for (std::size_t i = 0; i < agents.size(); ++i) {
        agents[i].region = i % 5;
        agents[i].alive = true;
    }

    std::vector<std::uint32_t> regionPopulations(5, 20);
    std::vector<std::array<double, 4>> regionBeliefs(5, {0, 0, 0, 0});

    // Run updates
    for (int i = 0; i < 10; ++i) {
        economy.update(regionPopulations, regionBeliefs, agents, i, nullptr);
    }

    double globalWelfare = economy.globalWelfare();
    double globalInequality = economy.globalInequality();

    // Sanity checks
    EXPECT_GE(globalWelfare, 0.0);
    EXPECT_GE(globalInequality, 0.0);
    EXPECT_LE(globalInequality, 1.0);
}
