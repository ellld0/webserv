#include  "LocationConfig.hpp"

Location::Location() : path_(""),
                     root_(""),
                     methods_(),
                     directoryListing_(false),
                     redirectCode_(0),
                     redirectUrl_(""),
                     index_(),
                     uploadPath_("")
                     {}


void Location::setPath(const std::string& path)
{
    path_ = path;
}

const std::string& Location::getPath() const
{
    return path_;
}

void Location::setRoot(const std::string& root)
{
    root_ = root;
}

const std::string& Location::getRoot() const
{
    return root_;
}

void Location::setMethods(const std::vector<std::string>& methods)
{
    methods_ = methods;
}

const std::vector<std::string>& Location::getMethods() const
{
    return methods_;
}

void Location::setDirectoryListing(bool directoryListing)
{
    directoryListing_ = directoryListing;
}

bool Location::getDirectoryListing() const
{
    return directoryListing_;
}

void Location::setRedirectCode(int code)
{
    redirectCode_ = code;
}

int Location::getRedirectCode() const
{
    return redirectCode_;
}

void Location::setRedirectUrl(const std::string& url)
{
    redirectUrl_ = url;
}

const std::string& Location::getRedirectUrl() const
{
    return redirectUrl_;
}

void Location::setIndex(const std::string& index)
{
    index_ = index;
}

const std::string& Location::getIndex() const
{
    return index_;
}

void Location::setUploadPath(const std::string& path)
{
    uploadPath_ = path;
}

const std::string& Location::getUploadPath() const
{
    return uploadPath_;
}
