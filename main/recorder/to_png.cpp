/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   Product name: redemption, a FLOSS RDP proxy
 *   Copyright (C) Wallix 2010-2012
 *   Author(s): Christophe Grosjean, Dominique Lafages, Jonathan Poelen
 */

#include "to_png.hpp"
#include "wrm_recorder_option.hpp"
#include "timer_compute.hpp"
#include "staticcapture.hpp"

void to_png(WRMRecorder& recorder, const char* outfile,
            std::size_t start, std::size_t stop, std::size_t interval,
            unsigned resize_width, unsigned resize_height,
            uint frame_limit,
            bool screenshot_start, bool no_screenshot_stop,
            bool screenshot_all)
{
    LOG(LOG_INFO, "to png -> %s", outfile);

    StaticCapture capture(recorder.meta().width,
                          recorder.meta().height,
                          outfile,
                          resize_width, resize_height, true);

    recorder.consumer(&capture);
    recorder.load_png_context(capture.drawable);
    TimerCompute timercompute(recorder);
    if (start && !timercompute.advance_second(start))
        return /*0*/;
    if (screenshot_start)
        capture.dump_png();

    uint frame = 0;
    const uint64_t coeff_sec_to_usec = 1000000;
    uint64_t mtime = coeff_sec_to_usec * interval;
    uint64_t msecond = coeff_sec_to_usec * (stop - start);
    uint64_t minterval = 0;

    while (recorder.reader.selected_next_order())
    {
        if (recorder.chunk_type() == WRMChunk::TIMESTAMP) {
            timercompute.chunk_time_value = recorder.reader.stream.in_uint64_be();
            timercompute.micro_sec += timercompute.chunk_time_value;
            --recorder.remaining_order_count();
            uint64_t usec = timercompute.usec();
            if (usec >= mtime)
            {
                capture.dump_png();
                timercompute.micro_sec = 0;
                if (++frame == frame_limit){
                    break;
                }
                if (screenshot_all){
                    minterval += usec - mtime;
                    while (minterval >= mtime){
                        capture.dump_png();
                        minterval -= mtime;
                    }
                }
            }

            if (msecond <= usec){
                msecond = 0;
                break;
            }
            else{
                msecond -= usec;
            }
        }
        else{
            recorder.interpret_order();
        }
    }
    if (!no_screenshot_stop && msecond && frame != frame_limit){
        capture.dump_png();
        //++frame;
    }
    LOG(LOG_INFO, "to png -> %s done", outfile);
    //return frame;
}

void to_png_2(WRMRecorder& recorder, const char* outfile,
            const std::vector<relative_time_point>& capture_points,
            unsigned resize_width, unsigned resize_height,
            bool no_screenshot_stop)
{
    if (capture_points.empty())
        return ;

    StaticCapture capture(recorder.meta().width,
                          recorder.meta().height,
                          outfile,
                          resize_width, resize_height, true);
    recorder.consumer(&capture);
    recorder.load_png_context(capture.drawable);

    typedef std::vector<relative_time_point>::const_iterator iterator;
    iterator it = capture_points.begin();

    TimerCompute timercompute(recorder);
    if (it->point.time && !timercompute.advance_second(it->point.time))
        return;

    capture.dump_png();
    iterator end = capture_points.end();
    if (++it == end)
        return;
    uint64_t mtime = 0;
    const uint64_t coeff_sec_to_usec = 1000000;

    while (recorder.reader.selected_next_order())
    {
        if (recorder.chunk_type() == WRMChunk::TIMESTAMP) {
            timercompute.chunk_time_value = recorder.reader.stream.in_uint64_be();
            timercompute.micro_sec += timercompute.chunk_time_value;
            --recorder.remaining_order_count();
            mtime += timercompute.usec();
            while (mtime >= coeff_sec_to_usec * it->point.time)
            {
                capture.dump_png();
                if (++it == end)
                    return;
            }
            timercompute.micro_sec = 0;
        }
        else
        {
            recorder.interpret_order();
        }
    }
    if (!no_screenshot_stop){
        capture.dump_png();
    }
}
