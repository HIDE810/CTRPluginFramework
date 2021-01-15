#include "CTRPluginFramework/Sound.hpp"
#include "CTRPluginFrameworkImpl/Sound/SoundImpl.hpp"

namespace CTRPluginFramework
{
    static inline void* ImplToPublic(SoundImpl* impl)
    {
        return static_cast<void*>(impl);
    }

    static inline SoundImpl* PublicToImpl(void* publ)
    {
        return static_cast<SoundImpl*>(publ);
    }

    Sound::Sound()
    {
        _soundImpl = nullptr;
    }

    Sound::Sound(const Sound &sound)
    {
        SoundImpl* otherImpl = PublicToImpl(sound._soundImpl);

        _soundImpl = sound._soundImpl;
        AtomicIncrement(&otherImpl->_refcount);
    }

    Sound::Sound(Sound&& sound) noexcept
    {
        _soundImpl = sound._soundImpl;
        sound._soundImpl = nullptr;
    }

    Sound::Sound(const std::string& bcwavFile, int maxSimultPlays)
    {
        _soundImpl = ImplToPublic(new SoundImpl(bcwavFile, maxSimultPlays));
    }

    Sound::Sound(const u8* bcwavBuffer, int maxSimultPlays)
    {
        _soundImpl = ImplToPublic(new SoundImpl(bcwavBuffer, maxSimultPlays));
    }

    Sound& Sound::operator=(const Sound &sound)
    {
        SoundImpl* thisImpl = PublicToImpl(_soundImpl);
        SoundImpl* otherImpl = PublicToImpl(sound._soundImpl);

        if (thisImpl)
            AtomicDecrement(&thisImpl->_refcount);
        if (otherImpl)
            AtomicIncrement(&otherImpl->_refcount);

        _soundImpl = sound._soundImpl;

        if (thisImpl && thisImpl->_refcount == 0)
            delete thisImpl;

        return *this;
    }

    Sound&   Sound::operator=(Sound &&sound) noexcept
    {
        SoundImpl* thisImpl = PublicToImpl(_soundImpl);

        if (thisImpl)
            AtomicDecrement(&thisImpl->_refcount);

        _soundImpl = sound._soundImpl;
        sound._soundImpl = nullptr;

        if (thisImpl && thisImpl->_refcount == 0)
            delete thisImpl;

        return *this;
    }

    Sound::~Sound()
    {
        SoundImpl* thisImpl = PublicToImpl(_soundImpl);

        if (thisImpl)
        {
            AtomicDecrement(&thisImpl->_refcount);

            if (thisImpl->_refcount == 0)
                delete thisImpl;
        }
    }

    Sound::LoadStatus Sound::GetLoadStatus()
    {
        if (_soundImpl)
            return static_cast<Sound::LoadStatus>(PublicToImpl(_soundImpl)->GetLoadStatus());
        else
            return LoadStatus::NOT_ALLOCATED;
    }

    void Sound::SetVolume(float volume)
    {
        if (_soundImpl)
            PublicToImpl(_soundImpl)->SetVolume(volume);
    }

    float Sound::GetVolume()
    {
        if (_soundImpl)
            return PublicToImpl(_soundImpl)->GetVolume();
        else
            return 0.f;
    }

    void Sound::SetPan(float pan)
    {
        if (_soundImpl)
            PublicToImpl(_soundImpl)->SetPan(pan);
    }

    float Sound::GetPan()
    {
        if (_soundImpl)
            return PublicToImpl(_soundImpl)->GetPan();
        else
            return 0.f;
    }

    u32 Sound::GetChannelAmount()
    {
        if (_soundImpl)
            return PublicToImpl(_soundImpl)->GetChannelAmount();
        else
            return 0;
    }

    bool Sound::IsLooped()
    {
        if (_soundImpl)
            return PublicToImpl(_soundImpl)->IsLooped();
        else
            return false;
    }

    bool Sound::Play()
    {
        return Play(0);
    }

    bool Sound::Play(int monoChannel)
    {
        return Play(monoChannel, -1);
    }

    bool Sound::Play(int leftEarChannel, int rightEarChannel)
    {
        if (_soundImpl)
            return PublicToImpl(_soundImpl)->Play(leftEarChannel, rightEarChannel);
        else
            return false;
    }

    void Sound::Stop()
    {
        Stop(-1);
    }

    void Sound::Stop(int monoChannel)
    {
        Stop(monoChannel, -1);
    }

    void Sound::Stop(int leftEarChannel, int rightEarChannel)
    {
        if (_soundImpl)
            PublicToImpl(_soundImpl)->Stop(leftEarChannel, rightEarChannel);
    }
}