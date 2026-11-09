[العربية](../../ar/internals/tls.md) | **English**

# TLS and Cryptography in std

`std.tls` is a TLS 1.3 client and server written in SultanC, on top of cryptography, encoding and
X.509 code that also lives in std. No C code, OpenSSL or host TLS library is involved. This page
describes the design, the protocol profile, the code owners, the security properties of the
implementation and its limits. It is written for contributors and for users who need to know what
the library does and does not guarantee.

Status: implemented, built and interoperability-tested. It has had no independent security review and
no side-channel measurement, so it is not declared production-secure; treat it as a candidate until it
has been audited.

## Design

The public `std.tls` API (`tls_client_connect`, `tls_server_accept`, `tls_read`, `tls_write`,
`tls_close` and the configuration functions) is implemented entirely in std. The code is built with
both the Native and the LLVM backend and produces the same behavior. `std.http` uses it for HTTPS and
for ALPN dispatch between HTTP/2 and HTTP/1.1.

## Owners

| Concern | Owner |
|---|---|
| OS randomness (getrandom, getentropy, random_get, ProcessPrng) | `library/std/os/entropy.sn` and the platform providers it imports |
| Word arithmetic, constant-time equality | `library/std/crypto/words.sn`; 64-bit words for SHA-512 in `crypto/words64.sn` |
| SHA-1, SHA-256, SHA-384/512 | `crypto/sha1.sn`, `crypto/sha256.sn`, `crypto/sha512.sn` |
| HMAC, HKDF, PBKDF2 over every digest | `crypto/digest.sn` |
| ChaCha20-Poly1305 | `crypto/chacha20_poly1305.sn` |
| AES, AES-CBC decryption | `crypto/aes.sn` |
| AES-GCM | `crypto/aes_gcm.sn` |
| GF(2^255-19) arithmetic | `crypto/field25519.sn` (shared by X25519 and Ed25519) |
| X25519 | `crypto/x25519.sn` |
| Ed25519 signing and verification | `crypto/ed25519.sn` |
| Montgomery multiprecision arithmetic | `crypto/bignum.sn` |
| RSA (PKCS#1 v1.5 and PSS verification, PSS signing) | `crypto/rsa.sn` |
| ECDSA P-256/P-384 verification, constant-time signing (RFC 6979) and ECDH | `crypto/ecdsa.sn` |
| 3DES-CBC and RC2-CBC decryption (legacy PKCS#12 only) | `crypto/des.sn`, `crypto/rc2.sn` |
| DER reading and encoding, OIDs from dotted text, BER-to-DER normalization | `library/std/encoding/der.sn` |
| PEM | `library/std/encoding/pem.sn` |
| X.509 parsing | `library/std/tls/x509.sn` |
| Path validation, host-name and name-constraint checks | `tls/verify.sn` |
| System trust anchors | `tls/trust.sn` |
| Record layer, key schedule, alerts | `tls/record.sn` |
| TLS presentation-language reading, scheme and group constants | `tls/wire.sn` |
| Ephemeral key shares (X25519, secp256r1, secp384r1) | `tls/key_exchange.sn` |
| CertificateVerify checking | `tls/signature.sn` (server-side signing: `tls/server.sn`) |
| Client handshake | `tls/client.sn` |
| Server handshake | `tls/server.sn` |
| PKCS#12 and PKCS#8 identities | `tls/pkcs12.sn` |
| Stream API | `tls/stream.sn` |
| Status codes | `tls/errors.sn` |
| ALPN (RFC 7301) | `tls/config.sn` (protocol lists), `tls/client.sn` and `tls/server.sn` (extension), `tls/stream.sn` (negotiated protocol) |

The consumers are `std.http.client` (HTTPS), the HTTP/2 client and the HTTPS server of `std.http`
(ALPN), user programs, the network TLS qualification test
(`tests/en/runtime/darwin/network_tls_*.sn`) and the live HTTP/2 check of the runner
(`tests/en/runtime/network/http2_live_*.sn`).

## Protocol profile

### Versions, suites and key exchange

- TLS 1.3 only (RFC 8446). TLS 1.2 servers are refused.
- Cipher suites: TLS_CHACHA20_POLY1305_SHA256, TLS_AES_128_GCM_SHA256 and TLS_AES_256_GCM_SHA384.
  The client offers them in that order and the server prefers the same order.
- Key exchange: X25519, secp256r1 and secp384r1. The client sends an X25519 share and lists all
  three. It answers a HelloRetryRequest for another supported group, or a cookie. The server takes
  its preferred group among the client's shares; otherwise it sends one HelloRetryRequest for its
  preferred group that the client supports.
- Signatures in CertificateVerify: ed25519, ecdsa_secp256r1_sha256, ecdsa_secp384r1_sha384 and
  rsa_pss_rsae_sha256/384/512. Certificates may additionally be signed with PKCS#1 v1.5
  SHA-256/384/512, RSASSA-PSS or Ed25519. SHA-1 and MD5 signatures are rejected.
- No PSK, resumption, 0-RTT, post-handshake client authentication or record padding on output.
  Middlebox compatibility mode (a session ID and one dummy change_cipher_spec) is used, as in Go and
  Rustls.

### ALPN

ALPN (RFC 7301) follows Go's `NextProtos`. A client configuration's `البروتوكولات` list
(`tls_client_config_set_protocols`, names of 1 to 255 bytes) is offered in the ClientHello. The
selected protocol must be one name the client offered, in EncryptedExtensions, or the handshake fails
with illegal_parameter.

A server with a list picks its own first protocol that the client offered and answers in
EncryptedExtensions. When the client offered ALPN and nothing matches, the handshake fails with
no_application_protocol (alert 120, status `tls_error_no_application_protocol`, -2014). A server without a
list, or a client offering none, ignores ALPN. The agreed protocol is
`tls_negotiated_protocol` (Go's `ConnectionState.NegotiatedProtocol`).
`std.http` offers `h2` and `http/1.1` and dispatches on the result.

### Certificate verification

The client builds a chain from the leaf through the peer's intermediates to the configured roots or,
when none are configured, the system bundle (`SSL_CERT_FILE` or the Go bundle locations on Linux,
`/etc/ssl/cert.pem` on macOS). It enforces validity periods, basicConstraints, keyUsage keyCertSign,
path length, EKU along the path, DNS and IP name constraints, and unknown critical extensions.

The server name is matched against SAN dNSName (a single left-most wildcard label) or SAN iPAddress;
the common name is ignored. Disabling peer verification (`tls_client_config_set_verify(false)`) still
checks the CertificateVerify signature against the leaf key, as Go's InsecureSkipVerify does.

### Server identity and client certificates

A server identity is a PKCS#12 container (DER or BER) with PBES2/PBKDF2/AES-CBC, legacy 3DES/RC2 PBE
or no encryption, and its MAC when present, holding an RSA, ECDSA P-256/P-384 or Ed25519 key
(PKCS#8). The CertificateVerify scheme is the one the key allows among the client's
signature_algorithms. The server verifies client certificates. The client sends an empty Certificate
when asked, because there is no client identity API.

## Security properties of the implementation

- Secret-dependent operations avoid secret-dependent branches and table indices.
  - AES SubBytes is computed by bitsliced GF(2^8) inversion.
  - GHASH, Poly1305 and X25519 use masked selection.
  - Ed25519 uses a fixed 256-step ladder with masked swaps over the unified Edwards addition law.
  - ECDSA signing and ECDH use the complete projective addition law (Renes, Costello and Batina 2016,
    Algorithm 4) in a fixed double-and-add-always ladder with masked selection, and Fermat inverses by
    fixed-window exponentiation.
  - RSA private operations use a fixed-window Montgomery exponentiation with a masked table scan over
    exponents padded to the prime length, followed by a public-key check of the signature.
  - 3DES and RC2 S-box and key-table reads scan whole tables.
- Verification-only operations (RSA public, ECDSA and Ed25519 verification) are variable time over
  public inputs. ECDSA nonce candidates outside [1, n) are rejected by a variable-time comparison
  (probability below 2^-32).
- AEAD tags and Finished values are compared in constant time, and no plaintext is released before the
  tag verifies.
- Randomness comes only from the OS CSPRNG. There is no fallback.

## Interoperability testing

The following was run on x86_64 Linux with executables produced by the SultanC compiler, with both
the Native and the LLVM backend.

- Crypto vectors: Ed25519 (RFC 8032 tests 1 and 3, and a tampered message rejected), X25519
  (RFC 7748 section 6.1), and ECDSA P-256/SHA-256 and P-384/SHA-384 signing equal to RFC 6979
  deterministic signatures (cross-checked with Python `cryptography`). Public-key derivation, ECDH
  shared secrets and rejection of an off-curve peer key are covered as well.
- The runner's TLS qualification programs `tests/en/runtime/darwin/network_tls_{server,client}.sn`
  (exit 42 on both sides) for every server and client backend pairing, with the runner's RSA-2048
  certificate recipe and with RSA, P-256, P-384 and Ed25519 identities.
- OpenSSL 3 interoperability. SultanC's server against `openssl s_client` (certificate verified) for
  TLS_AES_128_GCM_SHA256, TLS_AES_256_GCM_SHA384 and TLS_CHACHA20_POLY1305_SHA256; RSA-PSS, ECDSA and
  Ed25519 CertificateVerify; X25519, secp256r1 and secp384r1; and HelloRetryRequest when the client's
  only share is an unsupported group. SultanC's client against `openssl s_server` for the same
  suites, all four key types, and HelloRetryRequest to secp256r1 and secp384r1. Python `ssl` (OpenSSL)
  client and server.
- Negative case: the client fails against a certificate outside its trust roots (exit 4) and sends
  unknown_ca.
- PKCS#12: OpenSSL 3 containers (PBES2/AES-256-CBC), legacy 3DES/RC2, MAC-less unencrypted containers
  and BER-encoded containers (indefinite lengths, segmented OCTET STRINGs at every level) are accepted
  by the server.

## Limitations

- Not audited, and no timing measurements. Constant-time behavior is by construction of the source;
  the compiler gives no constant-time guarantee for generated code.
- Performance: arithmetic uses generic 32-bit limbs and unoptimized code. On the test host one full
  handshake with an RSA-2048 server key takes about 0.5 s per side with either backend, P-256
  operations take of the order of 0.1 s, and P-384 is slower.
- AES-GCM is slow because AES is bitsliced and constant-time in generic code. A peer that selects an
  AES suite (OpenSSL servers do) moves about ten kilobytes of application data per second.
  ChaCha20-Poly1305, which `std.tls` prefers, is much faster.
- Windows has no system trust anchors (CryptoAPI store access is not implemented) and needs explicit
  roots; WASI likewise. Linking `bcryptprimitives` and `crypt32` belongs to a Windows system-library
  link configuration that the target owner does not provide. The Windows path has not been run on a
  Windows host.
- There is no client identity: client certificates are verified by the server only.
- Name constraints other than dNSName and iPAddress subtrees make the constraining certificate
  unusable (fail closed).
- Certificate revocation (CRL and OCSP) is not checked, as in the defaults of Go and Rustls.

## References

RFC 8446 (TLS 1.3), RFC 7301 (ALPN), RFC 5280 (X.509), RFC 9525 and 6125 (service identity),
RFC 8439 (ChaCha20-Poly1305), RFC 7748 (X25519), RFC 8032 and 8410 (Ed25519), RFC 6979
(deterministic ECDSA), RFC 5915 (EC private keys), X.690 (BER/DER), RFC 8017 (PKCS#1), RFC 5869
(HKDF), RFC 8018 (PKCS#5), RFC 7292 (PKCS#12), RFC 5958 (PKCS#8), FIPS 180-4, FIPS 197,
NIST SP 800-38D, FIPS 186-5, SEC 1 and 2, RFC 2268 (RC2) and SP 800-67 (3DES).

Mature implementations consulted for structure and policy: Go `crypto/tls` and `crypto/x509`,
Rustls and webpki, TweetNaCl (field arithmetic layout, Ed25519 structure), poly1305-donna, and the
Renes-Costello-Batina complete addition formulas.

## See also

- [Platform Dependencies](../reference/platform-dependencies.md)
- [std coverage](../reference/std-coverage.md)
