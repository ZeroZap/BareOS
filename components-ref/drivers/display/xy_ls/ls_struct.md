## ## 整体

- dev
  - strip（为整体）
    - seg

```c
struct ls_dev {
    char * name;
    char 
}
```





## define_typedef

```c
#typdedef uint8_t mode_size_t
```



## 变量成员

```c
bool _running;
bool _triggered;
segment* _segments;                 // array of segments (20 bytes per element)
segment_runtime* _segment_runtimes; // array of segment runtimes (16 bytes per element)
uint8_t* _active_segments;          // array of active segments (1 bytes per element)

uint8_t _segments_len = 0;          // size of _segments array
uint8_t _active_segments_len = 0;   // size of _segments_runtime and _active_segments arrays
uint8_t _num_segments = 0;          // number of configured segments in the _segments array

segment* _seg;                      // currently active segment (20 bytes)
segment_runtime* _seg_rt;           // currently active segment runtime (16 bytes)

uint16_t _seg_len;                  // num LEDs in the currently active segment
};
```





## Segment

```c
uint16_t start;
uint16_t stop;
uint16_t speed;
mode_size_t mode;
uint8_t options;
uint8_t colors
```



## Segment_rumtime

```c
uint64_t next_time;
uint32_t counter_mode_step;
uint32_t counter_mode_call;
uint32_t aux_param;
uint32_t aux_param2;
uint32_t aux_param3;
uint8_t *ext_data = NULL;
uint32_t ex_data_cnt;
```

