#ifndef CHARGELAB_OPEN_FIRMWARE_STORAGE_H
#define CHARGELAB_OPEN_FIRMWARE_STORAGE_H

#include <cstdio>
#include <string>

#include "openocpp/interface/element/storage_interface.h"

namespace chargelab {
    namespace detail {
        class CloseFileWrapper {
        public:
            CloseFileWrapper(std::FILE* file)
                : file_(file)
            {
            }

            ~CloseFileWrapper() {
                std::fclose(file_);
            }

        private:
            std::FILE* file_;
        };
    }

    class StorageFile : public StorageInterface {
    public:
        explicit StorageFile(std::string filename) : filename_(std::move(filename))
        {
        }

    public:
        // ShoreOCPP patch 0007: safe save. Writing in place ("w" empties the file first) lost every setting
        // (Wi-Fi, CSMS URL, station ID) when the station reset during a save, and settings are saved a few
        // seconds after every boot (bench 2026-11-07 and 11-09). Now the new content goes to "<name>.tmp", and
        // only a complete, closed file replaces the old one. A reset at any moment leaves the old file or the
        // new one; if it came between removing the old file and renaming, read() finds the ".tmp".
        bool read(std::function<bool(FILE *)> const& function) override {
            for (auto const& name : {filename_, tempName()}) {
                auto file = std::fopen(name.c_str(), "r");
                if (file == nullptr)
                    continue;

                detail::CloseFileWrapper wrapper{file};
                return function(file);
            }
            return false;
        }

        bool write(std::function<bool(FILE *)> const& function) override {
            auto const temp = tempName();
            auto file = std::fopen(temp.c_str(), "w");
            if (file == nullptr)
                return false;

            bool ok = function(file);
            ok = (std::fflush(file) == 0) && ok;
            ok = (std::fclose(file) == 0) && ok;
            if (!ok) {
                std::remove(temp.c_str());
                return false;
            }

            // SPIFFS cannot rename over an existing file: remove the old one first (read() covers the gap)
            std::remove(filename_.c_str());
            return std::rename(temp.c_str(), filename_.c_str()) == 0;
        }

    private:
        std::string tempName() const {
            return filename_ + ".tmp";
        }

        std::string filename_;
    };

    class StorageNull : public StorageInterface {
    public:
        bool read(std::function<bool(FILE *)> const&) override {
            return false;
        }

        bool write(std::function<bool(FILE *)> const&) override {
            return false;
        }
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_STORAGE_H
