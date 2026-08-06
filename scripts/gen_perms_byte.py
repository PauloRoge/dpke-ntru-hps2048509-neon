N = 509
p = [1, 3, 6, 15, 30, 63, 126, 252]


def build_perm(nperm):
    perm = []

    for j in range(N):
        idx = j

        for _ in range(nperm):
            if (idx & 1) == 0:
                idx = idx // 2
            else:
                idx = (idx + N) // 2

        perm.append(idx)

    return perm


def emit_array(name, values):
    print(f"static const char {name}[512] = {{")

    for i in range(0, 512, 12):
        row = values[i:i + 12]
        line = ", ".join(f"{x:3d}" for x in row)
        print(f"    {line},")

    print("};\n")


def main():
    print("#ifndef R2_FROBENIUS_PERMS_BYTE_H")
    print("#define R2_FROBENIUS_PERMS_BYTE_H\n")

    for nperm in p:
        perm = build_perm(nperm)
        pB = [0] * 512
        pb = [0] * 512
        
        for j in range(N):
            pB[j] = perm[j] // 8
            pb[j] = perm[j] % 8

        emit_array(f"r2_frob_pB_{nperm}", pB)
        emit_array(f"r2_frob_pb_{nperm}", pb)

    print("#endif")


if __name__ == "__main__":
    main()