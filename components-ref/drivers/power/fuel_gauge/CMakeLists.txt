# Fuel Gauge Drivers

# BQ27Z561
set(BQ27Z561_SOURCES xy_fg_bq27z561.c)

# BQ27Z746
set(BQ27Z746_SOURCES
    xy_fg_bq27z746.c
    xy_fg_bq27z746.h
)

# BQ40Z50
set(BQ40Z50_SOURCES
    xy_fg_bq40z50.c
    xy_fg_bq40z50.h
)

# MAX17043
set(MAX17043_SOURCES xy_fg_max17043.c)

# Collect all fuel gauge sources
set(FUEL_GAUGE_SOURCES
    ${BQ27Z561_SOURCES}
    ${BQ27Z746_SOURCES}
    ${BQ40Z50_SOURCES}
    ${MAX17043_SOURCES}
)

xy_add_srcs(${FUEL_GAUGE_SOURCES})
