#include "NeuralNet.hpp"
#include <ATen/ops/softmax.h>

NeuralNet::NeuralNet()
{
    fc0 = register_module("fc0", torch::nn::Linear(8, 32));
    fc1 = register_module("fc1", torch::nn::Linear(32, 32));
    fc2 = register_module("fc2", torch::nn::Linear(32, 32));
    fc3 = register_module("fc3", torch::nn::Linear(32, 32));
    fc4 = register_module("fc4", torch::nn::Linear(32, 32));
    fc5 = register_module("fc5", torch::nn::Linear(32, 11));
}

torch::Tensor NeuralNet::forward(torch::Tensor x)
{
    x = torch::relu(fc0(x));
    x = torch::relu(fc1(x));
    x = torch::relu(fc2(x));
    x = torch::relu(fc3(x));
    x = torch::relu(fc4(x));
    x = torch::softmax(fc5(x), 0);
    return x;
}

