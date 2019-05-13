# Pull in our test harness emulator
FROM cfretz244/libdart_ci:test_harness

# Copy the unit tests into the root directory
COPY unit_tests /root

# Run our bootstrap script
CMD /root/provision.sh
