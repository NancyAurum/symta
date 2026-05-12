int.fact = //factorial
| R = 1
| while Me>0: R *= Me-
| R

// factoradic representation of a number
factoradic N =
| Ys = []
| I = 2
| while N:
  | push N%I Ys
  | N /= I
  | I+
| Ys.f

/*
unfactoradic Xs =
| N = 0
| K = 1
| I = 2
| till Xs.end
  | X = pop Xs
  | N += X*K
  | K *= I
  | I+
| N
*/

perm Xs Fs =
| when Xs.n<2: ret Xs
| Ys = perm Xs.tail Fs.tail
| Ys.insert{Fs.head Xs.head}

list.permutation N =
| Xs = Me
| N = N % Xs.n.fact
| Fs = factoradic{N}
| when Fs.n<Xs.n-1: Fs <= Fs.pad{Xs.n-1 0}
| Fs = Fs.f
| perm Xs Fs

//times I 25: say "[I]:[permutation I [a b c]]"
