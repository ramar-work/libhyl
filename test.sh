
# finally the last slog...
echo "libhyl Test suite!"


# non-existent config
hypno-harness -l lib/libhyl.so -d examples/noconfig.com -u /asdf

# bad syntax config
#hypno-harness -l lib/libhyl.so -d examples/badconfig.com -u /asdf

# really large config (like 200 routes or something)
#hypno-harness -l lib/libhyl.so -d examples/badconfig.com -u /asdf

# static paths (small file, <25kb)
#hypno-harness -l lib/libhyl.so -d examples/stresstest.com -u /assets/tiny.jpg

# static paths (big file, >2mb)
#hypno-harness -l lib/libhyl.so -d examples/stresstest.com -u /assets/big.jpg 

# static paths (ridiculous file, >100mb)
#hypno-harness -l lib/libhyl.so -d examples/stresstest.com -u /assets/testimony.wav 2>err
