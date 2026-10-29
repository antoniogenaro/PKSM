#include "app/Session.hpp"

#include "domain/GameProfile.hpp"
#include "domain/PokemonBank.hpp"
#include "platform/SaveStorage.hpp"
#include "sav/Sav.hpp"
#include "ui/Text.hpp"
#include <exception>

namespace pksm::switchui
{
    Session::Session(std::unique_ptr<pksm::Sav> save, const GameProfile& profile, std::string path,
        std::optional<console::SaveLocation> console)
        : save_(std::move(save)),
          profile_(&profile),
          sourcePath_(std::move(path)),
          console_(std::move(console))
    {
    }

    Session::~Session() = default;

    std::unique_ptr<Session> Session::parse(std::shared_ptr<u8[]> data, size_t size,
        std::string path, std::optional<console::SaveLocation> console, std::string& error)
    {
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
        return std::unique_ptr<Session>(
            new Session(std::move(save), *profile, std::move(path), std::move(console)));
    }

    std::unique_ptr<Session> Session::open(const std::string& path, std::string& error)
    {
        std::shared_ptr<u8[]> data;
        size_t size = 0;
        if (!storage::readSaveFile(path, data, size, error))
        {
            return nullptr;
        }
        return parse(std::move(data), size, path, std::nullopt, error);
    }

    std::unique_ptr<Session> Session::openConsole(
        const console::SaveLocation& location, std::string& error)
    {
        std::shared_ptr<u8[]> data;
        size_t size = 0;
        if (!console::readSave(location, data, size, error))
        {
            return nullptr;
        }
        return parse(std::move(data), size, std::string{}, location, error);
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

    PokemonBank* Session::bank(std::string& error)
    {
        if (!bank_)
        {
            bank_ = PokemonBank::open(error);
        }
        return bank_.get();
    }

    bool Session::dirty() const
    {
        return dirty_ || (bank_ && bank_->dirty());
    }

    bool Session::exportChanges(std::string& outputPath, std::string& error)
    {
        outputPath.clear();
        if (dirty_)
        {
            bool written = false;
            save_->finishEditing();
            if (console_)
            {
                written = console::writeSave(*console_, save_->rawData().get(),
                    save_->getEntireLengthIncludingFooter(), outputPath, error);
            }
            else
            {
                storage::ExportFile output;
                if (storage::createExportFile(sourcePath_, output, error))
                {
                    written = storage::writeExportFile(output, save_->rawData().get(),
                        save_->getEntireLengthIncludingFooter(), error);
                    outputPath = output.path;
                }
            }
            save_->beginEditing();
            if (!written)
            {
                outputPath.clear();
                return false;
            }
            dirty_ = false;
        }
        // Only after the save, so the bank never holds less than the saved game expects.
        return !bank_ || !bank_->dirty() || bank_->save(error);
    }

    bool Session::exportWithMessage(std::string& message)
    {
        const bool bankPending = bank_ && bank_->dirty();
        std::string outputPath;
        if (!exportChanges(outputPath, message))
        {
            return false;
        }
        if (outputPath.empty())
        {
            message = tr(Text::BankSaved);
        }
        else if (console_)
        {
            message = tr(bankPending ? Text::SavedToConsoleWithBank : Text::SavedToConsole,
                {outputPath});
        }
        else
        {
            message = tr(bankPending ? Text::ExportedWithBank : Text::ExportedTo, {outputPath});
        }
        return true;
    }
}
