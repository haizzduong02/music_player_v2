#ifndef PLAYLIST_TRACK_LIST_CONTROLLER_H
#define PLAYLIST_TRACK_LIST_CONTROLLER_H

#include "app/controller/PlaylistController.h"
#include "app/model/Playlist.h"
#include "interfaces/ITrackListController.h"

/**
 * @brief Adapter for playlist tracks to implement ITrackListController
 */
class PlaylistTrackListController : public ITrackListController
{
  public:
    PlaylistTrackListController(PlaylistController *controller, std::shared_ptr<Playlist> playlist,
                                PlaybackController *playbackController)
        : controller_(controller), playlist_(playlist), playbackController_(playbackController)
    {
    }

    void playTrack(const std::vector<std::shared_ptr<MediaFile>> &context, size_t index) override
    {
        if (playbackController_ && playlist_)
        {
            playbackController_->setCurrentPlaylist(playlist_.get());
            playbackController_->playContext(context, index);
        }
    }

    // ... rest same ...
    void removeTracks(const std::set<std::string> &paths) override
    {
        if (controller_ && playlist_)
        {
            for (const auto &path : paths)
            {
                controller_->removeFromPlaylistByPath(playlist_->getName(), path);
            }
        }
    }

    void removeTrackByPath(const std::string &path) override
    {
        if (controller_ && playlist_)
        {
            controller_->removeFromPlaylistByPath(playlist_->getName(), path);
        }
    }

    void clearAll() override
    {
        if (playlist_ && controller_)
        {
            auto tracks = playlist_->getTracks();
            for (const auto &t : tracks)
            {
                controller_->removeFromPlaylistByPath(playlist_->getName(), t->getPath());
            }
        }
    }

  private:
    PlaylistController *controller_;
    std::shared_ptr<Playlist> playlist_;
    PlaybackController *playbackController_;
};

#endif // PLAYLIST_TRACK_LIST_CONTROLLER_H
