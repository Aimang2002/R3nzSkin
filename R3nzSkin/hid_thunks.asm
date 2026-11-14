; Runtime forwarding thunks to the real system hid.dll.
; Export-name parity with hid.dll without a self-referential .def forwarder.
; The pointer table is defined here (native COFF) so the LTCG C++ object can
; reference it; C++ only declares it and fills it at load time.
.data
public hid_ptrs
hid_ptrs dq 44 dup(0)
.code
public hid_thunk_0
hid_thunk_0 proc
    jmp qword ptr [hid_ptrs + 0]
hid_thunk_0 endp
public hid_thunk_1
hid_thunk_1 proc
    jmp qword ptr [hid_ptrs + 8]
hid_thunk_1 endp
public hid_thunk_2
hid_thunk_2 proc
    jmp qword ptr [hid_ptrs + 16]
hid_thunk_2 endp
public hid_thunk_3
hid_thunk_3 proc
    jmp qword ptr [hid_ptrs + 24]
hid_thunk_3 endp
public hid_thunk_4
hid_thunk_4 proc
    jmp qword ptr [hid_ptrs + 32]
hid_thunk_4 endp
public hid_thunk_5
hid_thunk_5 proc
    jmp qword ptr [hid_ptrs + 40]
hid_thunk_5 endp
public hid_thunk_6
hid_thunk_6 proc
    jmp qword ptr [hid_ptrs + 48]
hid_thunk_6 endp
public hid_thunk_7
hid_thunk_7 proc
    jmp qword ptr [hid_ptrs + 56]
hid_thunk_7 endp
public hid_thunk_8
hid_thunk_8 proc
    jmp qword ptr [hid_ptrs + 64]
hid_thunk_8 endp
public hid_thunk_9
hid_thunk_9 proc
    jmp qword ptr [hid_ptrs + 72]
hid_thunk_9 endp
public hid_thunk_10
hid_thunk_10 proc
    jmp qword ptr [hid_ptrs + 80]
hid_thunk_10 endp
public hid_thunk_11
hid_thunk_11 proc
    jmp qword ptr [hid_ptrs + 88]
hid_thunk_11 endp
public hid_thunk_12
hid_thunk_12 proc
    jmp qword ptr [hid_ptrs + 96]
hid_thunk_12 endp
public hid_thunk_13
hid_thunk_13 proc
    jmp qword ptr [hid_ptrs + 104]
hid_thunk_13 endp
public hid_thunk_14
hid_thunk_14 proc
    jmp qword ptr [hid_ptrs + 112]
hid_thunk_14 endp
public hid_thunk_15
hid_thunk_15 proc
    jmp qword ptr [hid_ptrs + 120]
hid_thunk_15 endp
public hid_thunk_16
hid_thunk_16 proc
    jmp qword ptr [hid_ptrs + 128]
hid_thunk_16 endp
public hid_thunk_17
hid_thunk_17 proc
    jmp qword ptr [hid_ptrs + 136]
hid_thunk_17 endp
public hid_thunk_18
hid_thunk_18 proc
    jmp qword ptr [hid_ptrs + 144]
hid_thunk_18 endp
public hid_thunk_19
hid_thunk_19 proc
    jmp qword ptr [hid_ptrs + 152]
hid_thunk_19 endp
public hid_thunk_20
hid_thunk_20 proc
    jmp qword ptr [hid_ptrs + 160]
hid_thunk_20 endp
public hid_thunk_21
hid_thunk_21 proc
    jmp qword ptr [hid_ptrs + 168]
hid_thunk_21 endp
public hid_thunk_22
hid_thunk_22 proc
    jmp qword ptr [hid_ptrs + 176]
hid_thunk_22 endp
public hid_thunk_23
hid_thunk_23 proc
    jmp qword ptr [hid_ptrs + 184]
hid_thunk_23 endp
public hid_thunk_24
hid_thunk_24 proc
    jmp qword ptr [hid_ptrs + 192]
hid_thunk_24 endp
public hid_thunk_25
hid_thunk_25 proc
    jmp qword ptr [hid_ptrs + 200]
hid_thunk_25 endp
public hid_thunk_26
hid_thunk_26 proc
    jmp qword ptr [hid_ptrs + 208]
hid_thunk_26 endp
public hid_thunk_27
hid_thunk_27 proc
    jmp qword ptr [hid_ptrs + 216]
hid_thunk_27 endp
public hid_thunk_28
hid_thunk_28 proc
    jmp qword ptr [hid_ptrs + 224]
hid_thunk_28 endp
public hid_thunk_29
hid_thunk_29 proc
    jmp qword ptr [hid_ptrs + 232]
hid_thunk_29 endp
public hid_thunk_30
hid_thunk_30 proc
    jmp qword ptr [hid_ptrs + 240]
hid_thunk_30 endp
public hid_thunk_31
hid_thunk_31 proc
    jmp qword ptr [hid_ptrs + 248]
hid_thunk_31 endp
public hid_thunk_32
hid_thunk_32 proc
    jmp qword ptr [hid_ptrs + 256]
hid_thunk_32 endp
public hid_thunk_33
hid_thunk_33 proc
    jmp qword ptr [hid_ptrs + 264]
hid_thunk_33 endp
public hid_thunk_34
hid_thunk_34 proc
    jmp qword ptr [hid_ptrs + 272]
hid_thunk_34 endp
public hid_thunk_35
hid_thunk_35 proc
    jmp qword ptr [hid_ptrs + 280]
hid_thunk_35 endp
public hid_thunk_36
hid_thunk_36 proc
    jmp qword ptr [hid_ptrs + 288]
hid_thunk_36 endp
public hid_thunk_37
hid_thunk_37 proc
    jmp qword ptr [hid_ptrs + 296]
hid_thunk_37 endp
public hid_thunk_38
hid_thunk_38 proc
    jmp qword ptr [hid_ptrs + 304]
hid_thunk_38 endp
public hid_thunk_39
hid_thunk_39 proc
    jmp qword ptr [hid_ptrs + 312]
hid_thunk_39 endp
public hid_thunk_40
hid_thunk_40 proc
    jmp qword ptr [hid_ptrs + 320]
hid_thunk_40 endp
public hid_thunk_41
hid_thunk_41 proc
    jmp qword ptr [hid_ptrs + 328]
hid_thunk_41 endp
public hid_thunk_42
hid_thunk_42 proc
    jmp qword ptr [hid_ptrs + 336]
hid_thunk_42 endp
public hid_thunk_43
hid_thunk_43 proc
    jmp qword ptr [hid_ptrs + 344]
hid_thunk_43 endp
end
