add_test(
    NAME
        TestRemoteConnection
    COMMAND
        $<TARGET_FILE:ParaView::smTestDriver>
        --server $<TARGET_FILE:ParaView::pvserver>
        --client $<TARGET_FILE:PVAPIRemoteConnectionExample>
        --test
)
