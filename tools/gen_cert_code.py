import sys

def pem_to_c_string(file_path, var_name):
    with open(file_path, 'r') as f:
        content = f.read()
    content = content.replace("\\", "\\\\").replace("\n", "\\n\"\n\"")
    return f'static const char *{var_name} =\n"{content}\\n";\n'

device_name = sys.argv[1] if len(sys.argv) > 1 else "bau-001"

print("#pragma once\n")

# ⚡ A NOVA VARIÁVEL DINÂMICA AQUI!
print(f'static const char *DEVICE_ID = "{device_name}";\n')

# CAMINHOS ATUALIZADOS PARA O MONOREPO!
print(pem_to_c_string('emqx-broker/certs/cacert.pem', 'root_ca'))
print(pem_to_c_string(f'emqx-broker/certs/{device_name}.crt', 'client_cert'))
print(pem_to_c_string(f'emqx-broker/certs/{device_name}.key', 'client_key'))