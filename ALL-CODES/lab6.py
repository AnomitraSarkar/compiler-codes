
def shift_reduce_parser(input_string):
    tokens = input_string.split() + ["$"]  # add end marker
    stack = ["$"]

    # reduction rules
    reductions = {
        ("id",): "F",
        ("F",): "T",
        ("T",): "E",
        ("E", "+", "T"): "E",
        ("T", "*", "F"): "T"
    }

    i = 0
    print(f"{'Stack':<30}{'Input':<30}{'Action'}")

    while True:
        print(f"{' '.join(stack):<30}{' '.join(tokens[i:]):<30}", end="")

        if stack == ["$", "E"] and tokens[i] == "$":
            print("Accept ✅")
            break

        reduced = False
        for length in range(3, 0, -1):  # longest first
            rhs = tuple(stack[-length:])
            if rhs in reductions:
                lhs = reductions[rhs]

                # ✨ precedence handling: delay E → E + T if * comes next
                if rhs == ("E", "+", "T") and tokens[i] == "*":
                    continue

                print(f"Reduce by {lhs} → {' '.join(rhs)}")
                stack = stack[:-length] + [lhs]
                reduced = True
                break

        if reduced:
            continue

        if i < len(tokens):
            print(f"Shift {tokens[i]}")
            stack.append(tokens[i])
            i += 1
        else:
            print("Reject ❌")
            break


# ---- Test ----
expr = "id + id * id"
shift_reduce_parser(expr)
