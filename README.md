# PyPBC

PyPBC is a Python wrapper for the PBC (Pairing-Based Cryptography) library, allowing for easy use of pairing-based cryptography in Python.

***Note: Since the PBC library hasn't been maintained for a long time, there are some security issues, and the performance is lower than the current alternatives. Therefore, unless you have special requirements, it is not recommended to use this library. You can consider switching to [pymcl](https://github.com/Jemtaly/pymcl), which is a Python wrapper for the faster and securer [mcl](https://github.com/herumi/mcl) library, and supports the Windows platform. It currently provides support for the BLS12-381 curve only.***

## Installation

For Debian-based systems, you can simply install the package using the provided `install.sh` script.

For other systems, please refer to the instructions [here](INSTALL).

## Basic Usage

The library provides a simple interface to the PBC library, allowing for easy use of pairing-based cryptography in Python. Here is are some examples of how to use the library.

```python
from pypbc import *

# Initialize a set of parameters from a string 
# check the PBC documentation (http://crypto.stanford.edu/pbc/manual/) for more information
params = Parameters(
    "type a\n"
    "q 8780710799663312522437781984754049815806883199414208211028653399266475630880222957078625179422662221423155858769582317459277713367317481324925129998224791\n"
    "h 12016012264891146079388821366740534204802954401251311822919615131047207289359704531102844802183906537786776\n"
    "r 730750818665451621361119245571504901405976559617\n"
    "exp2 159\n"
    "exp1 107\n"
    "sign1 1\n"
    "sign0 1\n"
)

# Initialize the pairing
pairing = Pairing(params)

# show the order of the pairing
print(pairing.order())

# Generate random elements
g1 = Element.random(pairing, G1)
g2 = Element.random(pairing, G2)
z1 = Element.random(pairing, Zr)
z2 = Element.random(pairing, Zr)

# Check the properties of the pairing
assert pairing.apply(g1 ** z1, g2 ** z2) == pairing.apply(g1, g2) ** (z1 * z2)
```

## Method list

The following methods are available in the `pypbc` module:

### `Parameters`

- `__init__(self, string: str) -> None`: Initialize the parameters from a string.
- `__str__(self) -> str`: Return the string representation of the parameters.

### `Pairing`
    
- `__init__(self, params: Parameters) -> None`: Initialize the pairing from the given parameters.
- `order(self) -> int`: Return the order of the pairing (Zr, G1, G2 and GT).
- `apply(self, e1: Element, e2: Element) -> Element`: Apply the pairing to the given elements.
- `is_symmetric(self) -> bool`: Return whether the pairing is symmetric.

### `Element`
    
#### Constructors

- `__init__(self, pairing: Pairing, type: int, string: str) -> None`: Initialize the element from a string.
- `from_int(pairing: Pairing, value: int) -> Element`: Return an element in Zr from the given integer.
- `random(pairing: Pairing, type: int) -> Element`: Return a random element of the given type.
- `zero(pairing: Pairing, type: int) -> Element`: Return the additive identity element of the given type.
- `one(pairing: Pairing, type: int) -> Element`: Return the multiplicative identity element of the given type.
- `from_hash(pairing: Pairing, type: int, data: bytes) -> Element`: Return an element from the given hash.

#### Serialize and Deserialize

- `to_bytes(self) -> bytes`: Return the byte representation of the element.
- `to_bytes_compressed(self) -> bytes`: Return the compressed byte representation of the element. (Only for G1 and G2 elements)
- `to_bytes_x_only(self) -> bytes`: Return the x-only byte representation of the element. (Only for G1 and G2 elements)
- `from_bytes(pairing: Pairing, type: int, data: bytes) -> Element`: Return an element from the given byte representation.
- `from_bytes_compressed(pairing: Pairing, type: int, data: bytes) -> Element`: Return an element from the given compressed byte representation. (Only for G1 and G2 elements)
- `from_bytes_x_only(pairing: Pairing, type: int, data: bytes) -> Element`: Return an element from the given x-only byte representation. (Only for G1 and G2 elements)

#### Properties

- `order(self)`: Return the order of the element.
- `__getitem__(self, index: int) -> Element`: Return the i-th item of the element (if it is multidimensional).
- `__len__(self) -> int`: Return the length of the element, returns 0 if the element is one-dimensional.

#### Typecasting

- `__str__(self) -> str`: Return the string representation of the element.
- `__int__(self) -> int`: Return the integer representation of the element (if possible).

#### Hashable

- `__hash__(self) -> int`: Return the hash value of the element.

#### Arithmetic Operations

- `__add__(self, other: Element) -> Element`: Return the sum of the elements.
- `__sub__(self, other: Element) -> Element`: Return the difference of the elements.
- `__mul__(self, other: Element | int) -> Element`: Return the product of the elements, same as `__add__` method if the two operands are both in G1, G2 or GT, and same as `__pow__` if one of the operands is an integer or an element of Zr and another is in G1, G2 or GT.
- `__truediv__(self, other: Element) -> Element`: Return the quotient of the elements, the two operands must be in same field, it is same as `__sub__` if the two operands are both in G1, G2 or GT. ***Notice: Division between elements in G1,G2 or GT and elements in Zr is not allowed because this is not supported by the original pbc library. If you want to perform this operation, you can multiply the inverse of the Zr element (`g * ~x`) instead.***
- `__pow__(self, other: Element | int) -> Element`: Return the power of the element, the exponent can be an integer or an element of Zr.
- `__neg__(self) -> Element`: Return the additive inverse of the element.
- `__invert__(self) -> Element`: Return the multiplicative inverse of the element, same as `__neg__` if the element is in G1, G2 or GT.

#### Comparison Operations

- `__eq__(self, other: Element) -> bool`: Return whether the elements are equal.
- `__ne__(self, other: Element) -> bool`: Return whether the elements are not equal.
- `is0(self) -> bool`: Return whether the element is the additive identity.
- `is1(self) -> bool`: Return whether the element is the multiplicative identity.
