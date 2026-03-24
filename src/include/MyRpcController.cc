#include "MyRpcController.h"

MyRpcController::MyRpcController() {
    is_failed_ = false;
    error_text_ = "";
}

MyRpcController::~MyRpcController() = default;

void MyRpcController::Reset() {
    is_failed_ = false;
    error_text_ = "";
}

bool MyRpcController::Failed() const{
    return is_failed_;
}

std::string MyRpcController::ErrorText() const{
    return error_text_;
}

void MyRpcController::SetFailed(const std::string& error_text){
    is_failed_ = true;
    error_text_ = error_text;
}

void MyRpcController::NotifyOnCancel(google::protobuf::Closure* callback) {}

void MyRpcController::StartCancel() {}

bool MyRpcController::IsCanceled() const {return false;}
