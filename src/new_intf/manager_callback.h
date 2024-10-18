//
// Created by w00847030 on 2024/10/11.
//

#ifndef OEAWARE_MANAGER_CALLBACK_H
#define OEAWARE_MANAGER_CALLBACK_H
#include "base_data.h"
namespace oeaware {
class ManagerCallback {
public:
    virtual int Subscribe(const Topic &topic) = 0;
    virtual void Unsubscribe(const Topic &topic) = 0;
    virtual void Publish(const DataList &dataList) = 0;
};
}
#endif //OEAWARE_MANAGER_CALLBACK_H
