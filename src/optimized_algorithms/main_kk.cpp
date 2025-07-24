#include <iostream>
#include <vector>
#include <random>

// Structure for a box with ID and weight
struct Box {
    int id;
    int weight;
    Box(int i, int w) : id(i), weight(w) {}
};

int main() {
    const int N = 10; // Number of boxes
    std::vector<Box> boxes;
    boxes.reserve(N);

    // Random number generation setup
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> weight_dist(1, 100); // weights between 1 and 100

    // Create N random boxes
    for (int i = 0; i < N; ++i) {
        int w = weight_dist(gen);
        boxes.emplace_back(i, w);
    }

    // Print the boxes
    std::cout << "BoxID\tWeight\n";
    for (const auto& box : boxes) {
        std::cout << box.id << "\t" << box.weight << "\n";
    }

    return 0;
}