#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

#include <string>
#include <vector>

class LocationConfig {

	private:
		std::string path_;
		std::string root_;
		std::vector<std::string> methods_;
		bool directoryListing_;
		int redirectCode_;
		std::string redirectUrl_;
		std::string index_;
		std::string uploadPath_;
	public:
		LocationConfig();
		LocationConfig(const LocationConfig& other);
		LocationConfig& operator=(const LocationConfig& other);
		~LocationConfig();
		void setPath(const std::string& path);
		const std::string& getPath() const;
		void setRoot(const std::string& root);
		const std::string& getRoot() const;
		void setMethods(const std::vector<std::string>& methods);
		const std::vector<std::string>& getMethods() const;
		void setDirectoryListing(bool directoryListing);
		bool getDirectoryListing() const;
		void setRedirectCode(int code);
		int getRedirectCode() const;
		void setRedirectUrl(const std::string& url);
		const std::string& getRedirectUrl() const;
		void setIndex(const std::string& index);
		const std::string& getIndex() const;
		void setUploadPath(const std::string& path);
		const std::string& getUploadPath() const;

};

#endif // LOCATIONCONFIG_HPP