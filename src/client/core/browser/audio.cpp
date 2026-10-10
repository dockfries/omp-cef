#include "audio.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <AL/alc.h>

namespace
{
    const char* AlErrorToString(ALenum err)
    {
        switch (err) {
        case AL_NO_ERROR: return "AL_NO_ERROR";
        case AL_INVALID_NAME: return "AL_INVALID_NAME";
        case AL_INVALID_ENUM: return "AL_INVALID_ENUM";
        case AL_INVALID_VALUE: return "AL_INVALID_VALUE";
        case AL_INVALID_OPERATION: return "AL_INVALID_OPERATION";
        case AL_OUT_OF_MEMORY: return "AL_OUT_OF_MEMORY";
        default: return "AL_UNKNOWN_ERROR";
        }
    }

    const char* AlcErrorToString(ALCenum err)
    {
        switch (err) {
        case ALC_NO_ERROR: return "ALC_NO_ERROR";
        case ALC_INVALID_DEVICE: return "ALC_INVALID_DEVICE";
        case ALC_INVALID_CONTEXT: return "ALC_INVALID_CONTEXT";
        case ALC_INVALID_ENUM: return "ALC_INVALID_ENUM";
        case ALC_INVALID_VALUE: return "ALC_INVALID_VALUE";
        case ALC_OUT_OF_MEMORY: return "ALC_OUT_OF_MEMORY";
        default: return "ALC_UNKNOWN_ERROR";
        }
    }

    bool CheckAlError(const char* where, int browserId = -1)
    {
        ALenum err = alGetError();
        if (err != AL_NO_ERROR) {
            if (browserId >= 0) {
                LOG_ERROR("OpenAL error at {} browserId={} err={} ({})",
                    where, browserId, static_cast<int>(err), AlErrorToString(err));
            } else {
                LOG_ERROR("OpenAL error at {} err={} ({})",
                    where, static_cast<int>(err), AlErrorToString(err));
            }
            return false;
        }
        return true;
    }

    bool CheckAlcError(ALCdevice* device, const char* where)
    {
        if (!device)
            return true;

        ALCenum err = alcGetError(device);
        if (err != ALC_NO_ERROR) {
            LOG_ERROR("OpenAL ALC error at {} err={} ({})",
                where, static_cast<int>(err), AlcErrorToString(err));
            return false;
        }
        return true;
    }

    std::string GetDeviceName(ALCdevice* device)
    {
        if (!device)
            return "<null>";

        const ALCchar* name = alcGetString(device, ALC_DEVICE_SPECIFIER);
        if (!name)
            return "<unknown>";

        return std::string(name);
    }
}

bool AudioManager::Initialize()
{
    if (device_) {
        return true;
    }

    LOG_INFO("Initializing OpenAL audio system...");

    device_ = alcOpenDevice(nullptr);
    if (!device_) {
        LOG_ERROR_EX("Failed to open OpenAL audio device");
        return false;
    }

    LOG_INFO("OpenAL device opened: {}", GetDeviceName(static_cast<ALCdevice*>(device_)));
    CheckAlcError(static_cast<ALCdevice*>(device_), "alcOpenDevice");

    context_ = alcCreateContext(static_cast<ALCdevice*>(device_), nullptr);
    if (!context_) {
        LOG_ERROR_EX("Failed to create OpenAL audio context");
        CheckAlcError(static_cast<ALCdevice*>(device_), "alcCreateContext");

        alcCloseDevice(static_cast<ALCdevice*>(device_));
        device_ = nullptr;
        return false;
    }

    if (!alcMakeContextCurrent(static_cast<ALCcontext*>(context_))) {
        LOG_ERROR_EX("Failed to make OpenAL context current");
        CheckAlcError(static_cast<ALCdevice*>(device_), "alcMakeContextCurrent");

        alcDestroyContext(static_cast<ALCcontext*>(context_));
        context_ = nullptr;

        alcCloseDevice(static_cast<ALCdevice*>(device_));
        device_ = nullptr;
        return false;
    }

    alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
    CheckAlError("alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED)");

    LOG_INFO("OpenAL initialized successfully");

    terminate_ = false;
    audio_thread_ = std::thread(&AudioManager::AudioThreadLoop, this);

    return true;
}

void AudioManager::ShutdownBeforeProcessDetach()
{
    if (terminate_.load(std::memory_order_acquire) || !audio_thread_.joinable())
        return;

    // Signal first: by the time Shutdown() runs the thread may already be gone.
    terminate_.store(true, std::memory_order_release);
    cv_.notify_all();

    // SA-MP is deinitialising, so the process is still alive, this thread holds no loader lock and
    // the game keeps running - a wait here is safe and gives the thread its cleanup (and its
    // "OpenAL context and device destroyed" log line) instead of leaving libcef-style ghosts behind.
    constexpr auto kDrainTimeout = std::chrono::milliseconds(500);
    const bool stopped_in_time = WaitForAudioThread(kDrainTimeout);

    if (!stopped_in_time) {
        LOG_WARN("[Audio] The audio thread did not stop within {} ms while the process was still "
                 "alive - the detach path leaves it to the process exit.", kDrainTimeout.count());
        return;
    }

    if (audio_thread_.joinable() && audio_thread_.get_id() != std::this_thread::get_id())
        audio_thread_.join();

    LOG_INFO("[Audio] The audio system stopped before the process detach.");
}

bool AudioManager::WaitForAudioThread(std::chrono::milliseconds timeout)
{
    if (stopped_.load(std::memory_order_acquire))
        return true;

    std::unique_lock<std::mutex> lock(queue_mutex_);
    return cv_.wait_for(lock, timeout, [this] {
        return stopped_.load(std::memory_order_acquire);
    });
}

void AudioManager::Shutdown()
{
    // Repeat calls only have to finish an earlier one that had to give up on the join.
    const bool already_signalled = terminate_.exchange(true);
    if (already_signalled && !audio_thread_.joinable()) {
        return;
    }

    if (!already_signalled) {
        LOG_INFO("Shutting down audio system...");
    }

    cv_.notify_all();

    // This is reached twice: first from App::Shutdown() while the process is still healthy, and again
    // from the detach path in DllMain(PROCESS_DETACH), where the loader lock is held.
    //
    // The audio thread must never be waited for in the second case. It can be inside an OpenAL call
    // whose own mixing thread needs that loader lock, which is why the exit used to hang right after
    // this message, and why the first attempt at a fix - a one second grace period, then a detach -
    // still did not help: the OpenAL teardown below then blocked on the very same lock.
    //
    // So: only a thread that has already reported itself stopped is joined (that join cannot block),
    // anything else is detached and OpenAL is left to the process exit.
    bool stopped_cleanly = !audio_thread_.joinable();

    if (audio_thread_.joinable() && audio_thread_.get_id() != std::this_thread::get_id()) {
        const bool running = !stopped_.load(std::memory_order_acquire);

        if (running) {
            LOG_DEBUG("[Audio] The audio thread did not stop within the wait - detaching it.");
            audio_thread_.detach();
        } else {
            audio_thread_.join();
            stopped_cleanly = true;
        }
    }

    // Release OpenAL once nothing can use it any more. Without this the device (and its mixing
    // thread) stays open for the rest of the process, and a later Initialize() would report success
    // while the mixer thread that consumes the queue is gone for good.
    //
    // Only when the audio thread is gone: if it is still alive (detached above), the context and the
    // device belong to it, and touching them here is what hangs the exit.
    if (!stopped_cleanly) {
        LOG_DEBUG("[Audio] The audio thread is still alive - OpenAL is left to the process exit.");
        return;
    }

    alcMakeContextCurrent(nullptr);

    if (context_) {
        alcDestroyContext(static_cast<ALCcontext*>(context_));
        context_ = nullptr;
    }

    if (device_) {
        alcCloseDevice(static_cast<ALCdevice*>(device_));
        device_ = nullptr;
    }
}

void AudioManager::OnPcmPacket(int browserId, const float** data, int frames, int channels, int sampleRate)
{
    if (!device_ || !context_ || terminate_) {
        return;
    }

    if (frames <= 0 || channels <= 0 || !data) {
        return;
    }

    AudioPacket packet;
    packet.browserId = browserId;
    packet.sampleRate = sampleRate;
    packet.pcm_data.resize(frames);

    if (channels == 1) {
        for (int i = 0; i < frames; ++i) {
            const float s = std::clamp(data[0][i], -1.0f, 1.0f);
            packet.pcm_data[i] = static_cast<int16_t>(s * 32767.0f);
        }
    } else {
        for (int i = 0; i < frames; ++i) {
            float mix = 0.0f;
            for (int ch = 0; ch < channels; ++ch) {
                mix += data[ch][i];
            }
            mix /= static_cast<float>(channels);
            mix = std::clamp(mix, -1.0f, 1.0f);
            packet.pcm_data[i] = static_cast<int16_t>(mix * 32767.0f);
        }
    }

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);

        // The audio thread drains this every few milliseconds. If it ever falls behind - or died
        // (see AudioThreadLoop) - keep the newest audio instead of growing without bound.
        constexpr size_t kMaxQueuedPackets = 256;
        while (packet_queue_.size() >= kMaxQueuedPackets)
            packet_queue_.pop();

        packet_queue_.push(std::move(packet));
    }

    cv_.notify_one();
}

void AudioManager::EnsureStream(int browserId)
{
    std::lock_guard<std::mutex> lock(streams_mutex_);
    streams_.try_emplace(browserId);
}

void AudioManager::RemoveStream(int browserId)
{
    std::lock_guard<std::mutex> lock(streams_mutex_);

    auto it = streams_.find(browserId);
    if (it != streams_.end()) {
        it->second.pending_destroy.store(true);
    }
}

void AudioManager::UpdateSourcePosition(int browserId, float x, float y, float z)
{
    std::lock_guard<std::mutex> lock(streams_mutex_);

    auto& stream = streams_[browserId];
    stream.pos_x.store(x);
    stream.pos_y.store(y);
    stream.pos_z.store(z);
    stream.position_dirty.store(true);
}

void AudioManager::UpdateListenerPosition(float x, float y, float z, float at_x, float at_y, float at_z, float up_x, float up_y, float up_z)
{
    listener_pos_x_.store(x);
    listener_pos_y_.store(y);
    listener_pos_z_.store(z);

    listener_at_x_.store(-at_x);
    listener_at_y_.store(-at_y);
    listener_at_z_.store(-at_z);

    listener_up_x_.store(up_x);
    listener_up_y_.store(up_y);
    listener_up_z_.store(up_z);

    listener_dirty_.store(true);
}

void AudioManager::SetStreamMuted(int browserId, bool muted)
{
    std::lock_guard<std::mutex> lock(streams_mutex_);
    streams_[browserId].is_muted.store(muted);
}

void AudioManager::SetStreamAudioSettings(int browserId, float max_distance, float ref_distance)
{
    std::lock_guard<std::mutex> lock(streams_mutex_);

    auto& stream = streams_[browserId];
    stream.max_distance.store(max_distance);
    stream.ref_distance.store(ref_distance);
    stream.settings_dirty.store(true);
}

void AudioManager::SetStreamAudioMode(int browserId, AudioMode mode)
{
    std::lock_guard<std::mutex> lock(streams_mutex_);

    auto& stream = streams_[browserId];
    stream.audio_mode.store(mode);
    stream.settings_dirty.store(true);

    if (mode == AUDIO_MODE_UI) {
        stream.is_culled.store(false);
        stream.position_dirty.store(false);
    }
}

void AudioManager::AudioThreadLoop()
{
    // Whatever way this loop leaves - terminate_, the context failing to become current, an
    // exception - the thread has to report that it is gone, because Shutdown() joins it on that
    // answer. Everything below runs before the flag is set.
    struct StopReport
    {
        std::atomic<bool>& flag;
        ~StopReport() { flag.store(true, std::memory_order_release); }
    } stop_report{ stopped_ };

    if (!alcMakeContextCurrent(static_cast<ALCcontext*>(context_))) {
        LOG_ERROR_EX("Audio thread failed to make context current");
        CheckAlcError(static_cast<ALCdevice*>(device_), "AudioThreadLoop::alcMakeContextCurrent");

        // Nothing will drain the queue after this return, and OnPcmPacket only stops producing when
        // this flag is set.
        terminate_ = true;
        return;
    }

    alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
    CheckAlError("AudioThreadLoop::alDistanceModel");

    while (!terminate_) {
        {
            std::lock_guard<std::mutex> lock(streams_mutex_);

            if (listener_dirty_.exchange(false)) {
                alListener3f(AL_POSITION,
                    listener_pos_x_.load(),
                    listener_pos_y_.load(),
                    listener_pos_z_.load());
                CheckAlError("alListener3f(AL_POSITION)");

                float orientation[6] = {
                    listener_at_x_.load(), listener_at_y_.load(), listener_at_z_.load(),
                    listener_up_x_.load(), listener_up_y_.load(), listener_up_z_.load()
                };
                alListenerfv(AL_ORIENTATION, orientation);
                CheckAlError("alListenerfv(AL_ORIENTATION)");
            }

            std::vector<int> to_erase;
            to_erase.reserve(streams_.size());

            for (auto& [id, stream] : streams_) {
                if (stream.pending_destroy.load()) {
                    if (stream.source != 0) {
                        alSourceStop(stream.source);
                        CheckAlError("alSourceStop(pending_destroy)", id);

                        ALint queued = 0;
                        alGetSourcei(stream.source, AL_BUFFERS_QUEUED, &queued);
                        CheckAlError("alGetSourcei(AL_BUFFERS_QUEUED/pending_destroy)", id);

                        while (queued-- > 0) {
                            ALuint buf = 0;
                            alSourceUnqueueBuffers(stream.source, 1, &buf);
                            CheckAlError("alSourceUnqueueBuffers(pending_destroy)", id);
                        }

                        alDeleteSources(1, &stream.source);
                        CheckAlError("alDeleteSources(pending_destroy)", id);
                        stream.source = 0;

                        if (!stream.buffers.empty()) {
                            alDeleteBuffers((ALsizei)stream.buffers.size(), stream.buffers.data());
                            CheckAlError("alDeleteBuffers(pending_destroy)", id);
                            stream.buffers.clear();
                        }
                    }

                    to_erase.push_back(id);
                    continue;
                }

                if (stream.source == 0)
                    continue;

                if (stream.settings_dirty.exchange(false)) {
                    const int mode = stream.audio_mode.load();

                    if (mode == AUDIO_MODE_UI) {
                        alSourcei(stream.source, AL_SOURCE_RELATIVE, AL_TRUE);
                        CheckAlError("alSourcei(AL_SOURCE_RELATIVE=AL_TRUE)", id);

                        alSource3f(stream.source, AL_POSITION, 0.0f, 0.0f, 0.0f);
                        CheckAlError("alSource3f(AL_POSITION UI)", id);

                        stream.is_culled.store(false);
                    } else {
                        alSourcei(stream.source, AL_SOURCE_RELATIVE, AL_FALSE);
                        CheckAlError("alSourcei(AL_SOURCE_RELATIVE=AL_FALSE)", id);

                        stream.position_dirty.store(true);
                    }

                    alSourcef(stream.source, AL_MAX_DISTANCE, stream.max_distance.load());
                    CheckAlError("alSourcef(AL_MAX_DISTANCE)", id);

                    alSourcef(stream.source, AL_REFERENCE_DISTANCE, stream.ref_distance.load());
                    CheckAlError("alSourcef(AL_REFERENCE_DISTANCE)", id);

                    alSourcef(stream.source, AL_ROLLOFF_FACTOR, 4.0f);
                    CheckAlError("alSourcef(AL_ROLLOFF_FACTOR)", id);
                }

                if (stream.position_dirty.exchange(false)) {
                    alSource3f(stream.source, AL_POSITION,
                        stream.pos_x.load(),
                        stream.pos_y.load(),
                        stream.pos_z.load());
                    CheckAlError("alSource3f(AL_POSITION world)", id);
                }

                const float max_dist = stream.max_distance.load();
                const float dx = listener_pos_x_.load() - stream.pos_x.load();
                const float dy = listener_pos_y_.load() - stream.pos_y.load();
                const float dz = listener_pos_z_.load() - stream.pos_z.load();
                const float distance_sq = dx * dx + dy * dy + dz * dz;

                const bool should_be_culled =
                    (distance_sq > max_dist * max_dist) &&
                    (stream.audio_mode.load() == AUDIO_MODE_WORLD);

                const bool is_culled = stream.is_culled.load();

                if (should_be_culled && !is_culled) {
                    alSourcePause(stream.source);
                    CheckAlError("alSourcePause(cull)", id);
                    stream.is_culled.store(true);
                } else if (!should_be_culled && is_culled) {
                    alSourcePlay(stream.source);
                    CheckAlError("alSourcePlay(uncull)", id);
                    stream.is_culled.store(false);
                }
            }

            for (int id : to_erase) {
                streams_.erase(id);
            }
        }

        AudioPacket packet;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            constexpr auto timeout = std::chrono::milliseconds(16);

            if (cv_.wait_for(lock, timeout, [this] { return !packet_queue_.empty() || terminate_; })) {
                if (terminate_)
                    break;

                packet = std::move(packet_queue_.front());
                packet_queue_.pop();
            } else {
                continue;
            }
        }

        AudioStream* stream_ptr = nullptr;
        {
            std::lock_guard<std::mutex> lock(streams_mutex_);

            auto it = streams_.find(packet.browserId);
            if (it == streams_.end()) {
                continue;
            }

            if (it->second.is_muted.load() || it->second.pending_destroy.load()) {
                continue;
            }

            stream_ptr = &it->second;

            if (stream_ptr->source == 0) {
                alGenSources(1, &stream_ptr->source);
                if (!CheckAlError("alGenSources", packet.browserId) || stream_ptr->source == 0) {
                    LOG_ERROR("Failed to generate source for browserId={}", packet.browserId);
                    continue;
                }

                stream_ptr->buffers.resize(4, 0);
                alGenBuffers((ALsizei)stream_ptr->buffers.size(), stream_ptr->buffers.data());
                if (!CheckAlError("alGenBuffers", packet.browserId)) {
                    LOG_ERROR("Failed to generate buffers for browserId={}", packet.browserId);

                    alDeleteSources(1, &stream_ptr->source);
                    CheckAlError("alDeleteSources(after alGenBuffers failure)", packet.browserId);

                    stream_ptr->source = 0;
                    stream_ptr->buffers.clear();
                    continue;
                }

                stream_ptr->settings_dirty.store(true);
            }
        }

        ALint processed = 0;
        alGetSourcei(stream_ptr->source, AL_BUFFERS_PROCESSED, &processed);
        CheckAlError("alGetSourcei(AL_BUFFERS_PROCESSED)", packet.browserId);

        ALint queued_count = 0;
        alGetSourcei(stream_ptr->source, AL_BUFFERS_QUEUED, &queued_count);
        CheckAlError("alGetSourcei(AL_BUFFERS_QUEUED)", packet.browserId);

        if (processed == 0 && queued_count >= (ALint)stream_ptr->buffers.size()) {
            continue;
        }

        ALuint buffer_to_use = 0;

        if (processed > 0) {
            alSourceUnqueueBuffers(stream_ptr->source, 1, &buffer_to_use);
            if (!CheckAlError("alSourceUnqueueBuffers", packet.browserId)) {
                continue;
            }
        } else {
            if (queued_count < 0 || queued_count >= (ALint)stream_ptr->buffers.size()) {
                continue;
            }

            buffer_to_use = stream_ptr->buffers[queued_count];
        }

        alBufferData(
            buffer_to_use,
            AL_FORMAT_MONO16,
            packet.pcm_data.data(),
            (ALsizei)(packet.pcm_data.size() * sizeof(int16_t)),
            packet.sampleRate);
        if (!CheckAlError("alBufferData", packet.browserId)) {
            continue;
        }

        alSourceQueueBuffers(stream_ptr->source, 1, &buffer_to_use);
        if (!CheckAlError("alSourceQueueBuffers", packet.browserId)) {
            continue;
        }

        ALint state_after_queue = 0;
        alGetSourcei(stream_ptr->source, AL_SOURCE_STATE, &state_after_queue);
        CheckAlError("alGetSourcei(AL_SOURCE_STATE after queue)", packet.browserId);

        if (state_after_queue != AL_PLAYING && !stream_ptr->is_culled.load()) {
            alSourcePlay(stream_ptr->source);
            CheckAlError("alSourcePlay", packet.browserId);
        }
    }

    {
        std::lock_guard<std::mutex> lock(streams_mutex_);

        for (auto& [id, stream] : streams_) {
            if (stream.source != 0) {
                alSourceStop(stream.source);
                CheckAlError("alSourceStop(cleanup)", id);

                ALint queued = 0;
                alGetSourcei(stream.source, AL_BUFFERS_QUEUED, &queued);
                CheckAlError("alGetSourcei(AL_BUFFERS_QUEUED cleanup)", id);

                while (queued-- > 0) {
                    ALuint buf = 0;
                    alSourceUnqueueBuffers(stream.source, 1, &buf);
                    CheckAlError("alSourceUnqueueBuffers(cleanup)", id);
                }

                alDeleteSources(1, &stream.source);
                CheckAlError("alDeleteSources(cleanup)", id);
            }

            if (!stream.buffers.empty()) {
                alDeleteBuffers((ALsizei)stream.buffers.size(), stream.buffers.data());
                CheckAlError("alDeleteBuffers(cleanup)", id);
            }
        }

        streams_.clear();
    }

    alcMakeContextCurrent(nullptr);
    CheckAlcError(static_cast<ALCdevice*>(device_), "alcMakeContextCurrent(nullptr)");

    if (context_) {
        alcDestroyContext((ALCcontext*)context_);
        context_ = nullptr;
    }

    if (device_) {
        alcCloseDevice((ALCdevice*)device_);
        device_ = nullptr;
    }

    LOG_INFO("OpenAL context and device destroyed");
}