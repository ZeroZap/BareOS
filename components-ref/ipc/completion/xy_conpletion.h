#ifndef _XY_COMPLETION_H_
#define _XY_COMPLETION_H_

#ifdef __cplusplus
extern "C" {
#endif


ypedef struct {

} xy_completion_t;

xy_error_t xy_completion_wait(struct xy_completion *completion,
                              uint32_t timeout)

#ifdef __cplusplus
}
#endif

#endif