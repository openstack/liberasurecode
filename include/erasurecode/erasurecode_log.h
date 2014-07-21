/*
 * <Copyright>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.  THIS SOFTWARE IS PROVIDED BY
 * THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * liberasurecode logging routines
 *
 * vi: set noai tw=79 ts=4 sw=4:
 */

#ifndef _ERASURECODE_LOG_H_
#define _ERASURECODE_LOG_H_

/* ==~=*=~==~=*=~==~=*=~==~=*=~==~= Logging =~==~=*=~==~=*=~==~=*=~==~=*=~== */

#if __STDC_VERSION__ < 199901L
    #if __GNUC__ >= 2
        #define __func__ __FUNCTION__
    #else
        #define __func__ "<unknown>"
    #endif
#endif

#define _LOG1(level, ...) \
    syslog (level, __VA_ARGS__)

#define _LOG2(level, ...) \
    syslog (level, "%s:%d:%s\n", __FILE__, __LINE__, __VA_ARGS__)

#define log_info(...)  _LOG1(LOG_INFO, __VA_ARGS__)
#define log_warn(...)  _LOG1(LOG_WARNING, __VA_ARGS__)
#define log_error(...) _LOG1(LOG_ERR, __VA_ARGS__)
#define log_debug(...) _LOG2(LOG_DEBUG, __VA_ARGS__)

/* ==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~== */

#endif  // _ERASURECODE_LOG_H_
