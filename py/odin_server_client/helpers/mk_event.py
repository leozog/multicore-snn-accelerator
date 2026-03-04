def mk_single_synapse_event(pre_neur: int, post_neur: int) -> int:
    """Single-synapse event word: ADDR<16>=1, ADDR<15:8>=pre_neur, ADDR<7:0>=post_neur"""
    if not (0 <= pre_neur < 256 and 0 <= post_neur < 256):
        raise ValueError("pre_neur and post_neur must be 0..255")
    return (1 << 16) | (pre_neur << 8) | post_neur


def mk_single_neuron_time_ref(neur: int) -> int:
    """Single-neuron time reference event: ADDR<15:8>=neur, ADDR<7:0>=0xFF"""
    if not (0 <= neur < 256):
        raise ValueError("neur must be 0..255")
    return (neur << 8) | 0xFF


def mk_all_neurons_time_ref() -> int:
    """All-neurons time reference event: ADDR<7:0>=0x7F"""
    return 0x7F


def mk_single_neuron_bistability(neur: int) -> int:
    """Single-neuron bistability event: ADDR<15:8>=neur, ADDR<7:0>=0x80"""
    if not (0 <= neur < 256):
        raise ValueError("neur must be 0..255")
    return (neur << 8) | 0x80


def mk_all_neurons_bistability() -> int:
    """All-neurons bistability event: ADDR<7:0>=0x00 (word 0)."""
    return 0x00


def mk_neuron_spike_event(pre_neur: int) -> int:
    """Neuron spike event: ADDR<15:8>=pre_neur, ADDR<7:0>=0x07"""
    if not (0 <= pre_neur < 256):
        raise ValueError("pre_neur must be 0..255")
    return (pre_neur << 8) | 0x07


def mk_virtual_event(neur: int, weight: int, sign: int, leak: int) -> int:
    """Virtual event: ADDR<15:8>=neur, ADDR<7:0>=[7:5]=w<2:0>, [4]=s, [3]=l, [2:0]=001"""
    if not (0 <= neur < 256):
        raise ValueError("neur must be 0..255")
    if not (0 <= weight < 8):
        raise ValueError("weight must be 0..7")
    if sign not in (0, 1):
        raise ValueError("sign must be 0 or 1")
    if leak not in (0, 1):
        raise ValueError("leak must be 0 or 1")
    low = ((weight & 0x7) << 5) | ((sign & 0x1) << 4) | ((leak & 0x1) << 3) | 0x01
    return (neur << 8) | low

