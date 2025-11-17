#!/bin/bash
# Docker test suite for Grand Strategy Simulation Engine

set -e

echo "=========================================="
echo "Grand Strategy Kernel - Docker Tests"
echo "=========================================="
echo

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

IMAGE_NAME="grand-strategy-kernel:test"

# Build the image
echo -e "${YELLOW}Building Docker image...${NC}"
docker build -t $IMAGE_NAME . || {
    echo -e "${RED}✗ Build failed${NC}"
    exit 1
}
echo -e "${GREEN}✓ Build successful${NC}"
echo

# Test 1: Basic execution
echo -e "${YELLOW}Test 1: Basic execution${NC}"
echo "metrics" | docker run -i --rm $IMAGE_NAME > /dev/null && {
    echo -e "${GREEN}✓ Basic execution works${NC}"
} || {
    echo -e "${RED}✗ Basic execution failed${NC}"
    exit 1
}
echo

# Test 2: Step command
echo -e "${YELLOW}Test 2: Step command${NC}"
echo "step 10" | docker run -i --rm $IMAGE_NAME | grep -q "generation" && {
    echo -e "${GREEN}✓ Step command works${NC}"
} || {
    echo -e "${RED}✗ Step command failed${NC}"
    exit 1
}
echo

# Test 3: Metrics output
echo -e "${YELLOW}Test 3: Metrics output${NC}"
echo "metrics" | docker run -i --rm $IMAGE_NAME | grep -q "Polarization" && {
    echo -e "${GREEN}✓ Metrics output works${NC}"
} || {
    echo -e "${RED}✗ Metrics output failed${NC}"
    exit 1
}
echo

# Test 4: Volume mounting
echo -e "${YELLOW}Test 4: Volume mounting${NC}"
mkdir -p ./test_data
echo "run 100 10" | docker run -i --rm -v $(pwd)/test_data:/app/data $IMAGE_NAME > /dev/null
if [ -f "./test_data/metrics.csv" ]; then
    echo -e "${GREEN}✓ Volume mounting works${NC}"
    cat ./test_data/metrics.csv | head -5
    rm -rf ./test_data
else
    echo -e "${RED}✗ Volume mounting failed${NC}"
    exit 1
fi
echo

# Test 5: JSON export
echo -e "${YELLOW}Test 5: JSON export${NC}"
echo -e "step 10\nstate traits" | docker run -i --rm $IMAGE_NAME | grep -q '"traits"' && {
    echo -e "${GREEN}✓ JSON export with traits works${NC}"
} || {
    echo -e "${RED}✗ JSON export failed${NC}"
    exit 1
}
echo

# Test 6: Reset command
echo -e "${YELLOW}Test 6: Reset command${NC}"
echo -e "reset 1000 50 8 0.05\nmetrics" | docker run -i --rm $IMAGE_NAME | grep -q "Generation: 0" && {
    echo -e "${GREEN}✓ Reset command works${NC}"
} || {
    echo -e "${RED}✗ Reset command failed${NC}"
    exit 1
}
echo

# Test 7: Image size check
echo -e "${YELLOW}Test 7: Image size check${NC}"
SIZE=$(docker images $IMAGE_NAME --format "{{.Size}}")
echo "Image size: $SIZE"
echo -e "${GREEN}✓ Image built successfully${NC}"
echo

# Test 8: Container resource usage
echo -e "${YELLOW}Test 8: Container resource usage${NC}"
CONTAINER_ID=$(echo "run 100 10" | docker run -d -i $IMAGE_NAME)
sleep 2
docker stats --no-stream $CONTAINER_ID
docker stop $CONTAINER_ID > /dev/null
docker rm $CONTAINER_ID > /dev/null
echo -e "${GREEN}✓ Resource monitoring works${NC}"
echo

# Summary
echo "=========================================="
echo -e "${GREEN}All tests passed! ✓${NC}"
echo "=========================================="
echo
echo "You can now use the image:"
echo "  docker run -it --rm $IMAGE_NAME"
echo
echo "Or tag and push to a registry:"
echo "  docker tag $IMAGE_NAME your-registry/grand-strategy-kernel:latest"
echo "  docker push your-registry/grand-strategy-kernel:latest"
