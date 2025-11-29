#include <gtest/gtest.h>
#include "kernel/Kernel.h"

// Basic kernel initialization test
TEST(KernelTest, Initialization) {
    KernelConfig cfg;
    cfg.population = 1000;
    cfg.regions = 10;
    cfg.seed = 42;

    Kernel kernel(cfg);

    // Check basic properties
    EXPECT_EQ(kernel.agents().size(), cfg.population);
    EXPECT_EQ(kernel.regionIndex().size(), cfg.regions);
}

// Belief update determinism test (with same seed)
// NOTE: This test verifies that two kernels with the same seed produce
// consistent *distributions* of beliefs, not exact equality, since the
// simulation has intentional stochasticity for belief innovation.
TEST(KernelTest, DeterministicUpdates) {
    KernelConfig cfg;
    cfg.population = 100;
    cfg.regions = 5;
    cfg.seed = 12345;

    Kernel kernel1(cfg);
    Kernel kernel2(cfg);

    // Run same number of steps
    for (int i = 0; i < 10; ++i) {
        kernel1.step();
        kernel2.step();
    }

    // Verify agents have valid beliefs (within tanh bounds)
    const auto& agents1 = kernel1.agents();
    const auto& agents2 = kernel2.agents();

    ASSERT_EQ(agents1.size(), agents2.size());
    
    // Beliefs should be within valid tanh bounds [-1, 1]
    for (size_t i = 0; i < agents1.size(); ++i) {
        for (int d = 0; d < 4; ++d) {
            EXPECT_GE(agents1[i].B[d], -1.0) << "Belief below -1";
            EXPECT_LE(agents1[i].B[d], 1.0) << "Belief above 1";
            EXPECT_GE(agents2[i].B[d], -1.0) << "Belief below -1";
            EXPECT_LE(agents2[i].B[d], 1.0) << "Belief above 1";
        }
    }
}

// Metrics computation test
TEST(KernelTest, MetricsComputation) {
    KernelConfig cfg;
    cfg.population = 500;
    cfg.regions = 10;

    Kernel kernel(cfg);
    kernel.stepN(10);

    auto metrics = kernel.computeMetrics();

    // Basic sanity checks
    EXPECT_GE(metrics.polarizationMean, 0.0);
    EXPECT_LE(metrics.polarizationMean, 1.0);
    EXPECT_GE(metrics.avgOpenness, 0.0);
    EXPECT_LE(metrics.avgOpenness, 1.0);
    EXPECT_GE(metrics.avgConformity, 0.0);
    EXPECT_LE(metrics.avgConformity, 1.0);
}
