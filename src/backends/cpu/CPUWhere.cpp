
#include "CPUWhere.hpp"

namespace mllm {

CPUWhere::CPUWhere(Backend *bn, string opName, float data, int axis, int threadCount) :
    thread_count(threadCount),
    Op(bn, opName) {
    data_ = data;
    axis_ = (Chl)axis;
}

ErrorCode CPUWhere::reshape(vector<shared_ptr<Tensor>> inputs, vector<shared_ptr<Tensor>> outputs) {
    return Op::reshape(inputs, outputs);
}

ErrorCode CPUWhere::execute(vector<shared_ptr<Tensor>> inputs, vector<shared_ptr<Tensor>> outputs) {
    vector<float> b_vec = {};
    vector<float> s_vec = {};
    vector<float> h_vec = {};
    vector<float> d_vec = {};
#pragma omp parallel for collapse(4) num_threads(thread_count)
    for (int b = 0; b < inputs[0]->batch(); b++) {
        for (auto s = 0; s < inputs[0]->sequence(); s++) {
            for (auto h = 0; h < inputs[0]->head(); h++) {
                for (auto d = 0; d < inputs[0]->dimension(); d++) {
                    if (inputs[0]->dataAt<float>(b, h, h, s) == data_) {
                        b_vec.push_back(b);
                        s_vec.push_back(s);
                        h_vec.push_back(h);
                        d_vec.push_back(d);
                    }
                }
            }
        }
    }
    int num = b_vec.size();
    if (axis_ == -1) {
        outputs[0]->reshape(1, 1, 4, num);
        outputs[0]->setDtype(activation_dtype());
        outputs[0]->alloc();
        for (int i = 0; i < 4; ++i) {
            auto dest_ptr = outputs[0]->hostPtr<float>() + outputs[0]->offset(0, 0, i, 0);
            switch (i) {
            case 0:
                memcpy(dest_ptr, b_vec.data(), num * sizeof(float));
                break;
            case 1:
                memcpy(dest_ptr, h_vec.data(), num * sizeof(float));
                break;
            case 2:
                memcpy(dest_ptr, s_vec.data(), num * sizeof(float));
                break;
            case 3:
                memcpy(dest_ptr, d_vec.data(), num * sizeof(float));
                break;
            default:
                break;
            }
        }
    } else {
        outputs[0]->reshape(1, 1, 1, num);
        outputs[0]->setDtype(activation_dtype());
        outputs[0]->alloc();
        auto dest_ptr = outputs[0]->hostPtr<float>();
        switch (axis_) {
        case BATCH:
            memcpy(dest_ptr, b_vec.data(), num * sizeof(float));
            break;
        case HEAD:
            memcpy(dest_ptr, h_vec.data(), num * sizeof(float));
            break;
        case SEQUENCE:
            memcpy(dest_ptr, s_vec.data(), num * sizeof(float));
            break;
        case DIMENSION:
            memcpy(dest_ptr, d_vec.data(), num * sizeof(float));
            break;
        default:
            break;
        }
    }
    return Op::execute(inputs, outputs);
}
} // namespace mllm
