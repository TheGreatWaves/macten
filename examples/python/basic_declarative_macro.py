defmacten_dec list {
    () => {
        (l := list(),
         l.append(1),
         l.append(2),
         l.append(3),
         l
        )[-1]
    }
}

if __name__ == "__main__":
    print(list![])
