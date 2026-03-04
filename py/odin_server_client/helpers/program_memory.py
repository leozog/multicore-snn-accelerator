import numpy as np
from odin_server_client.odin_server_client import OdinServerClient
from odin_server_client.odin_synapse import OdinSynapse
from odin_server_client.odin_neuron import OdinNeuron

def program_synram(
    client: OdinServerClient,
    core_id: int,
    weights: np.ndarray,
    mapping_bit: int = 1,
):
    if not isinstance(weights, np.ndarray):
        raise TypeError("weights must be a numpy.ndarray")
    if weights.ndim != 2:
        raise ValueError("weights must be a 2D array with shape (out_features, in_features)")

    weights = np.clip(weights, 0, 7).astype(int)
    
    out_features, in_features = weights.shape

    commands = []
    for post_idx in range(out_features):
        for pre_idx in range(in_features):
            w_val = weights[post_idx, pre_idx]
            commands.append(OdinSynapse.make_set_cmd(core_id, pre_idx, post_idx, w_val, mapping_bit))

    if len(commands) == 0:
        print("Warn: No set synapse commands generated")
    
    return commands

def program_lif_neurons(
    client: OdinServerClient, 
    core_id: int,
    v_thr: np.ndarray,
    leak_str: np.ndarray,
):
    if v_thr.ndim != 1 or leak_str.ndim != 1 or v_thr.shape[0] != leak_str.shape[0]:
        raise ValueError("v_thr and leak_str must be 1D iterables of the same length")

    v_thr = np.clip(v_thr, 0, (1 << 8) - 1).astype(int)
    leak_str = np.clip(leak_str, 0, (1 << 7) - 1).astype(int)

    commands = []
    for neur_id, (t, l) in enumerate(zip(v_thr, leak_str)):
        neuron = {"lif_izh_sel": 1, "thr": int(t), "leak_str": int(l)}
        commands.append(OdinNeuron.make_set_cmd(core_id, neur_id, neuron))

    if len(commands) == 0:
        print("Warn: No set neuron commands generated")
    
    return commands