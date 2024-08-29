#pragma once
#include "CommonObject.h"
#include <string>

class DownloadThread
{
public:
    void operator()(CommonObjects& common);
    void SetUrl(std::string_view new_url);
private:
    std::string download_url;
};