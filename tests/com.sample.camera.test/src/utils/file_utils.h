#ifndef _FILE_UTILS_H_
#define _FILE_UTILS_H_

std::string getLastFile(const char *path, const char *ext);
void deleteFile(const std::string &file);
void deleteAll(const char *path, const char *ext);

#endif //_FILE_UTILS_H_