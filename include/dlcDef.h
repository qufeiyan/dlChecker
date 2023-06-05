/**
 * @file    dlcDef.h
 * @author  qufeiyan
 * @brief   
 * @version 1.0.0
 * @date    2023/05/16 18:50:31
 * @version Copyright (c) 2023
 */

/* Define to prevent recursive inclusion ---------------------------------------------------*/
#ifndef DLCDEF_H
#define DLCDEF_H
/* Include ---------------------------------------------------------------------------------*/
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __weak
#define __weak  __attribute__((weak))
#endif

#ifndef __unused 
#define __unused __attribute__ ((unused))
#endif

/* Indirect stringification.  Doing two levels allows the parameter to be a
 * macro itself.  For example, compile with -DFOO=bar, __stringify(FOO)
 * converts to "bar".
 */ 
#define __stringify_1(x...)	#x
#define __stringify(x...)	__stringify_1(x)

/**
 * @brief   Declare a module object in current context.
 *
 * @param   type is the module type.
 * @param   module is the name of module.
 * @return  NULL is default value.
 * @note    
 * @see     
 */
#define DECLARE_CURRENT_MODULE_OBJECT(type, module) \
    __weak typeof(type) *module##Current(){\
        return NULL;\
    }

/**
 * @brief   Define a module object in current context.
 *
 * @param   type is the module type.
 * @param   module is name of the module.
 * @param   instance is pointer to the module object.
 * @return  pointer to the module object.
 * @note    
 * @see     
 */
#define DEFINE_CURRENT_MODULE_OBJECT(type, module, instance) \
    typeof(type) *module##Current(){\
        type *_obj = (typeof(instance)) instance;\
        return _obj;\
    }

typedef int32_t err_t;

#define DLC_NAME_SIZE    (16)

#define MEM_ALIGNMENT    (sizeof(size_t))

/**
 * @ingroup BasicDef
 *
 * @def MEM_ALIGN_UP(size, align)
 * Return the most contiguous size aligned at specified width. MEM_ALIGN_UP(13, 4)
 * would return 16.
 */
#define MEM_ALIGN_UP(size, align)        (((size) + (align) - 1) & ~((align) - 1))

/**
 * @ingroup BasicDef
 *
 * @def MEM_ALIGN_DOWN(size, align)
 * Return the down number of aligned at specified width. MEM_ALIGN_DOWN(13, 4)
 * would return 12.
 */
#define MEM_ALIGN_DOWN(size, align)      ((size) & ~((align) - 1))


#if defined(__GNUC__)
/** Macro for getting minimum value. No sideeffects, a and b are evaluated once only. */
#define DLC_MIN(a, b)   ({  \
    typeof(a) _a = (a);     \
    typeof(b) _b = (b);     \
    _a < _b ? _a : _b;      \
})
/** Macro for getting maximum value. No sideeffects, a and b are evaluated once only. */
#define DLC_MAX(a, b)   ({  \
    typeof(a) _a = (a);     \
    typeof(b) _b = (b);     \
    _a > _b ? _a : _b;      \
})
#else
/** Macro for getting minimum value. */
#define DLC_MIN(a, b)    ((a) < (b) ? (a) : (b))
/** Macro for getting maximum value. */
#define DLC_MAX(a, b)    ((a) > (b) ? (a) : (b))
#endif

#ifdef __cplusplus
}
#endif

/**
 * fls - find last (most-significant) bit set
 * @x: the word to search
 *
 * This is defined the same way as ffs.
 * Note fls(0) = 0, fls(1) = 1, fls(0x80000000) = 32.
 */

static inline uint32_t __fls(uint32_t val)
{
    uint32_t  bit = 32;

    if (!val)
        return 0;
    if (!(val & 0xffff0000u))
    {
        val <<= 16;
        bit -= 16;
    }
    if (!(val & 0xff000000u))
    {
        val <<= 8;
        bit -= 8;
    }
    if (!(val & 0xf0000000u))
    {
        val <<= 4;
        bit -= 4;
    }
    if (!(val & 0xc0000000u))
    {
        val <<= 2;
        bit -= 2;
    }
    if (!(val & 0x80000000u))
    {
        bit -= 1;
    }

    return bit;
}


static inline bool is_power_of_2(unsigned long n)
{
	return (n != 0 && ((n & (n - 1)) == 0));
}

static inline uint32_t __roundup_pow_of_two(unsigned int x)
{
    return 1UL << __fls(x - 1);
}

static inline uint32_t __rounddown_pow_of_two(unsigned int x)
{
    return (1UL << __fls(x - 1)) / 2;
}

#endif	//  DLCDEF_H