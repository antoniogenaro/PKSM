#include "app/Session.hpp"

#include "domain/GameProfile.hpp"
#include "platform/SaveStorage.hpp"
#include "sav/Sav.hpp"
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
            error = std::string("No se pudo interpretar el guardado: ") + exception.what();
            return nullptr;
        }
        const GameProfile* profile = save ? findGameProfile(save->version()) : nullptr;
        if (!profile)
        {
            error = "El archivo no es un guardado compatible de Sword/Shield o Rojo Fuego/Verde Hoja.";
            return nullptr;
        }

        save->beginEditing();
        return std::unique_ptr<Session>(new Session(std::move(save), *profile, path));
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
}
