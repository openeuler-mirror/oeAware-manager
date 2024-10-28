/******************************************************************************
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#ifndef COMMON_SERIALIZE_H
#define COMMON_SERIALIZE_H
#include <vector>
#include <string>
#include <cstring>
#include <memory>
#include <sstream>

namespace oeaware {
class OutStream {
public:
    template<typename T>
    OutStream& operator<<(const T& data);
    template<typename T>
    OutStream& operator<<(const std::shared_ptr<T>& data);
    template<typename T>
    OutStream& operator<<(const std::vector<T> &data);
    template<typename T>
    OutStream& operator<<(const std::vector<std::shared_ptr<T>> &data);
    void Append(const char *data, size_t len)
    {
        os.write(data, len);
    }
    std::string Str()
    {
        return os.str();
    }
public:
    std::stringstream os;
};

class InStream {
public:
    InStream(const std::string &s) : is(s) { }
    template<typename T>
    InStream& operator>>(T& data);
    template<typename T>
    InStream& operator>>(std::shared_ptr<T> &data);
    template<typename T>
    InStream& operator>>(std::vector<T> &data);
    template<typename T>
    InStream& operator>>(std::vector<std::shared_ptr<T>> &data);
    void Deserialize(char *data, size_t len)
    {
        is.read(data, len);
    }
    std::string Str()
    {
        return is.str();
    }
private:
    std::stringstream is;
};


/* General template function */
template<typename T>
static void inline Deserialize(InStream &is, T &data)
{
    data.Deserialize(is);
}

template<typename T>
static void inline Serialize(oeaware::OutStream &buf, const T &data)
{
    data.Serialize(buf);
}

/* Serialize for base type */
/* char */
template<>
void inline Serialize(OutStream &buf, const char &data)
{
    buf.Append(reinterpret_cast<const char*>(&data), sizeof(const char));
}

template<>
void inline Deserialize(InStream &is, char &data)
{
    is.Deserialize(reinterpret_cast<char*>(&data), sizeof(char));
}

/* unsigned char */
template<>
void inline Serialize(OutStream &buf, const unsigned char &data)
{
    buf.Append(reinterpret_cast<const char*>(&data), sizeof(const unsigned char));
}

template<>
void inline Deserialize(InStream &is, unsigned char &data)
{
    is.Deserialize(reinterpret_cast<char*>(&data), sizeof(unsigned char));
}

/* short int */
template<>
void inline Serialize(OutStream &buf, const short int &data)
{
    buf.Append(reinterpret_cast<const char*>(&data), sizeof(const short int));
}

template<>
void inline Deserialize(InStream &is, short int &data)
{
    is.Deserialize(reinterpret_cast<char*>(&data), sizeof(short int));
}

/* unsigned short int */
template<>
void inline Serialize(OutStream &buf, const unsigned short int &data)
{
    buf.Append(reinterpret_cast<const char*>(&data), sizeof(const unsigned short int));
}

template<>
void inline Deserialize(InStream &is, unsigned short int &data)
{
    is.Deserialize(reinterpret_cast<char*>(&data), sizeof(unsigned short int));
}

/* unsigned int */
template<>
void inline Serialize(OutStream &buf, const unsigned int &data)
{
    buf.Append(reinterpret_cast<const char*>(&data), sizeof(const unsigned int));
}

template<>
void inline Deserialize(InStream &is, unsigned int &data)
{
    is.Deserialize(reinterpret_cast<char*>(&data), sizeof(unsigned int));
}

/* int */
template<>
void inline Serialize(OutStream &buf, const int &data)
{
    buf.Append(reinterpret_cast<const char*>(&data), sizeof(const int));
}

template<>
void inline Deserialize(InStream &is, int &data)
{
    is.Deserialize(reinterpret_cast<char*>(&data), sizeof(int));
}

/* long int */
template<>
void inline Serialize(OutStream &buf, const long int &data)
{
    buf.Append(reinterpret_cast<const char*>(&data), sizeof(const long int));
}

template<>
void inline Deserialize(InStream &is, long int &data)
{
    is.Deserialize(reinterpret_cast<char*>(&data), sizeof(long int));
}

/* unsigned long int */
template<>
void inline Serialize(OutStream &buf, const unsigned long int &data)
{
    buf.Append(reinterpret_cast<const char*>(&data), sizeof(const unsigned long int));
}

template<>
void inline Deserialize(InStream &is, unsigned long int &data)
{
    is.Deserialize(reinterpret_cast<char*>(&data), sizeof(unsigned long int));
}

/* long long int */
template<>
void inline Serialize(OutStream &buf, const long long int &data)
{
    buf.Append(reinterpret_cast<const char*>(&data), sizeof(const long long int));
}

template<>
void inline Deserialize(InStream &is, long long int &data)
{
    is.Deserialize(reinterpret_cast<char*>(&data), sizeof(long long int));
}

/* unsigned long long int */
template<>
void inline Serialize(OutStream &buf, const unsigned long long int &data)
{
    buf.Append(reinterpret_cast<const char*>(&data), sizeof(const unsigned long long int));
}

template<>
void inline Deserialize(InStream &is, unsigned long long int &data)
{
    is.Deserialize(reinterpret_cast<char*>(&data), sizeof(unsigned long long int));
}

/* float */
template<>
void inline Serialize(OutStream &buf, const float &data)
{
    buf.Append(reinterpret_cast<const char*>(&data), sizeof(const float));
}

template<>
void inline Deserialize(InStream &is, float &data)
{
    is.Deserialize(reinterpret_cast<char*>(&data), sizeof(float));
}

/* double */
template<>
void inline Serialize(OutStream &buf, const double &data)
{
    buf.Append(reinterpret_cast<const char*>(&data), sizeof(const double));
}

template<>
void inline Deserialize(InStream &is, double &data)
{
    is.Deserialize(reinterpret_cast<char*>(&data), sizeof(double));
}

/* Serialize for string */
template<>
void Serialize(OutStream &buf, const std::string &data)
{
    ::oeaware::Serialize(buf, data.length());
    buf.Append(data.data(), data.length());
}

template<>
void Deserialize(InStream &is, std::string &data)
{
    size_t len = 0;
    ::oeaware::Deserialize(is, len);
    data.resize(len);
    is.Deserialize(&data[0], len);
}

/* Serialize for char* */
template<>
void Serialize(OutStream &buf, const char* const &data)
{
    size_t len = strlen(data);
    ::oeaware::Serialize(buf, len);
    buf.Append(data, len);
}

template<>
void Deserialize(InStream &is, const char* &data)
{
    size_t len = 0;
    ::oeaware::Deserialize(is, len);
    auto tmp_data = new char[len + 1];
    is.Deserialize(tmp_data, len);
    tmp_data[len] = 0;
    data = tmp_data;
}

template<typename T>
 OutStream& OutStream::operator<<(const T& data)
 {
    ::oeaware::Serialize<T>(*this, data);
    return *this;
}

template<typename T>
OutStream& OutStream::operator<<(const std::shared_ptr<T>& data)
{
    ::oeaware::Serialize<T>(*this, *data);
    return *this;
}

template<typename T>
OutStream& OutStream::operator<<(const std::vector<T> &data)
{
    size_t len = data.size();
    ::oeaware::Serialize(*this, len);
    for (auto &i : data) {
        ::oeaware::Serialize<T>(*this, i);
    }
    return *this;
}

template<typename T>
OutStream& OutStream::operator<<(const std::vector<std::shared_ptr<T>> &data)
{
    size_t len = data.size();
    ::oeaware::Serialize(*this, len);
    for (auto &i : data) {
        ::oeaware::Serialize<T>(*this, *i);
    }
    return *this;
}

template<typename T>
InStream& InStream::operator>>(T& data)
{
    ::oeaware::Deserialize<T>(*this, data);
    return *this;
}

template<typename T>
InStream& InStream::operator>>(std::shared_ptr<T> &data)
{
    ::oeaware::Deserialize<T>(*this, *data);
    return *this;
}

template<typename T>
InStream& InStream::operator>>(std::vector<T> &data)
{
    size_t len = 0;
    ::oeaware::Deserialize(*this, len);
    for (size_t i = 0; i < len; ++i) {
        T temp;
        ::oeaware::Deserialize<T>(*this, temp);
        data.emplace_back(temp);
    }
    return *this;
}

template<typename T>
InStream& InStream::operator>>(std::vector<std::shared_ptr<T>> &data)
{
    size_t len = 0;
    ::oeaware::Deserialize(*this, len);
    for (size_t i = 0; i < len; ++i) {
        std::shared_ptr<T> temp = std::make_shared<T>();
        ::oeaware::Deserialize<T>(*this, *temp);
        data.emplace_back(temp);
    }
    return *this;
}

template<typename T>
std::string Encode(const T &msg)
{
    oeaware::OutStream out;
    out << msg;
    return out.Str();
}

template<typename T>
bool Decode(T &msg, const std::string &content)
{
    try {
        oeaware::InStream in(content);
        in >> msg;
    }  catch (std::exception &e) {
        return false;
    }
    return true;
}
}

#endif
