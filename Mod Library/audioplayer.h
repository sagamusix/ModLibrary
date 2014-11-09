/*
 * audioplayer.h
 * -------------
 * Purpose: Implementation of the Mod Library audio player thread.
 * Notes  : (currently none)
 * Authors: Johannes Schultz
 * The Mod Library source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include <QThread>
#include <QFile>
#include <libopenmpt/libopenmpt.hpp>
#include <portaudio.h>

class AudioThread : public QObject
{
	Q_OBJECT

protected:
	QByteArray content;
	openmpt::module mod;
	int volume;
public:
	volatile bool kill;

public:
	AudioThread(QFile &file, int v) : content(file.readAll()), mod(content.begin(), content.end()), kill(false)
	{
		mod.select_subsong(-1);	// Play all subsongs consecutively
		setVolume(v);
	}

public slots:
	void process()
	{
		const std::size_t buffersize = 4800;
		const std::int32_t samplerate = 48000;
		std::vector<float> left(buffersize);
		std::vector<float> right(buffersize);
		mod.set_repeat_count(-1);
		Pa_Initialize();
		PaStream * stream = 0;
		PaStreamParameters streamparameters;
		std::memset(&streamparameters, 0, sizeof(PaStreamParameters));
		streamparameters.device = Pa_GetDefaultOutputDevice();
		streamparameters.channelCount = 2;
		streamparameters.sampleFormat = paFloat32 | paNonInterleaved;
		streamparameters.suggestedLatency = Pa_GetDeviceInfo(streamparameters.device)->defaultHighOutputLatency;
		Pa_OpenStream(&stream, nullptr, &streamparameters, samplerate, paFramesPerBufferUnspecified, 0, nullptr, nullptr);
		Pa_StartStream(stream);
		while(!kill)
		{
			std::size_t count = mod.read(samplerate, buffersize, left.data(), right.data());
			if(count == 0)
			{
				break;
			}
			const float * const buffers [2] = { left.data(), right.data() };
			Pa_WriteStream(stream, buffers, count);
		}
		Pa_StopStream(stream);
		Pa_CloseStream(stream);
		Pa_Terminate();
		emit finished();
	}

	void setVolume(int v)
	{
		volume = v;
		mod.set_render_param(openmpt::module::RENDER_MASTERGAIN_MILLIBEL, (volume - 100) * 50);
	}

signals:
	void finished();

};
