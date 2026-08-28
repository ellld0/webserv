#include  "LocationConfig.hpp"

LocationConfig::LocationConfig() : path_(""),
                     root_(""),
                     methods_(),
                     directoryListing_(false),
                     redirectCode_(0),
                     redirectUrl_(""),
                     index_(),
                     uploadPath_("")
                     {}

LocationConfig::LocationConfig(const LocationConfig& other)
    : path_(other.path_),
      root_(other.root_),
      methods_(other.methods_),
      directoryListing_(other.directoryListing_),
      redirectCode_(other.redirectCode_),
      redirectUrl_(other.redirectUrl_),
      index_(other.index_),
      uploadPath_(other.uploadPath_)
{

}

LocationConfig& LocationConfig::operator=(const LocationConfig& other)
{
    if (this != &other)
    {
        path_ = other.path_;
        root_ = other.root_;
        methods_ = other.methods_;
        directoryListing_ = other.directoryListing_;
        redirectCode_ = other.redirectCode_;
        redirectUrl_ = other.redirectUrl_;
        index_ = other.index_;
        uploadPath_ = other.uploadPath_;
    }
    return *this;
}

LocationConfig::~LocationConfig()
{

}


void LocationConfig::setPath(const std::string& path)
{
    path_ = path;
}

const std::string& LocationConfig::getPath() const
{
    return path_;
}

void LocationConfig::setRoot(const std::string& root)
{
    root_ = root;
}

const std::string& LocationConfig::getRoot() const
{
    return root_;
}

void LocationConfig::setMethods(const std::vector<std::string>& methods)
{
    methods_ = methods;
}

const std::vector<std::string>& LocationConfig::getMethods() const
{
    return methods_;
}

void LocationConfig::setDirectoryListing(bool directoryListing)
{
    directoryListing_ = directoryListing;
}

bool LocationConfig::getDirectoryListing() const
{
    return directoryListing_;
}

void LocationConfig::setRedirectCode(int code)
{
    redirectCode_ = code;
}

int LocationConfig::getRedirectCode() const
{
    return redirectCode_;
}

void LocationConfig::setRedirectUrl(const std::string& url)
{
    redirectUrl_ = url;
}

const std::string& LocationConfig::getRedirectUrl() const
{
    return redirectUrl_;
}

void LocationConfig::setIndex(const std::string& index)
{
    index_ = index;
}

const std::string& LocationConfig::getIndex() const
{
    return index_;
}

void LocationConfig::setUploadPath(const std::string& path)
{
    uploadPath_ = path;
}

const std::string& LocationConfig::getUploadPath() const
{
    return uploadPath_;
}
