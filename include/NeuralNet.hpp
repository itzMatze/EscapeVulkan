#include <torch/nn/modules/linear.h>

class NeuralNet : public torch::nn::Module
{
public:
    NeuralNet();
    torch::Tensor forward(torch::Tensor x);
private:
    torch::nn::Linear fc0 = nullptr;
    torch::nn::Linear fc1 = nullptr;
    torch::nn::Linear fc2 = nullptr;
    torch::nn::Linear fc3 = nullptr;
    torch::nn::Linear fc4 = nullptr;
    torch::nn::Linear fc5 = nullptr;
};

