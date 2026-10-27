#include "app/Session.hpp"

#include "domain/GameProfile.hpp"
#include "platform/SaveStorage.hpp"
#include "sav/Sav.hpp"
#include "ui/Text.hpp"
#include <exception>

namespace pksm::switchui
{
    Session::Session(std::unique_ptr<pksm::Sav> save, const GameProfile& profile, std::string path)
        : save_(std::move(save)), profile_(&profile), sourcePath_(std::move(path))
    {
    }

    Session::~Session() = default;

    std::unique_ptr<Session> Session::open(const std::string& path, std::string& error)
    {
        std::shared_ptr<u8[]> data;
        size_t size = 0;
        if (!storage::readSaveFile(path, data, size, error))
        {
            return nullptr;
        }

        std::unique_ptr<pksm::Sav> save;
        try
        {
            save = pksm::Sav::getSave(data, size);
        }
        catch (const std::exception& exception)
        {
            error = tr(Text::SaveParseFailed, {exception.what()});
            return nullptr;
        }
        const GameProfile* profile = save ? findGameProfile(save->version()) : nullptr;
        if (!profile)
        {
            error = tr(Text::SaveUnsupported);
            return nullptr;
        }

        save->beginEditing();
        return std::unique_ptr<Session>(new Session(std::move(save), *profile, path));
    }

    std::optional<pksm::GameVersion> Session::identify(const std::string& path)
    {
        std::shared_ptr<u8[]> data;
        size_t size = 0;
        std::string error;
        if (!storage::readSaveFile(path, data, size, error))
        {
            return std::nullopt;
        }
        try
        {
            if (const auto save = pksm::Sav::getSave(data, size))
            {
                return save->version();
            }
        }
        catch (const std::exception&)
        {
        }
        return std::nullopt;
    }

    bool Session::exportChanges(std::string& outputPath, std::string& error)
    {
        storage::ExportFile output;
        if (!storage::createExportFile(sourcePath_, output, error))
        {
            return false;
        }
        outputPath = output.path;

        save_->finishEditing();
        const bool written = storage::writeExportFile(
            output, save_->rawData().get(), save_->getEntireLengthIncludingFooter(), error);
        save_->beginEditing();
        if (written)
        {
            dirty_ = false;
        }
        return written;
    }

    bool Session::exportWithMessage(std::string& message)
    {
        std::string outputPath;
        if (!exportChanges(outputPath, message))
        {
            return false;
        }
        message = tr(Text::ExportedTo, {outputPath});
        return true;
    }
}
