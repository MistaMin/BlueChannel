# Source and reference-material policy

Blue Channel's authored source, numerical netlists, behavioural models, calibration tables, tests and
UI artwork are OpenGrid project work distributed under MIT. Device parameters and model equations are
engineering approximations; the project does not distribute vendor SPICE implementation files or claim
its estimates are measurements of physical hardware.

The runtime models are `Source/Circuit/Netlists.h` and `Source/Circuit/HQAudioCircuit.h`. These numerical
implementations are distinct from the original hardware documentation used during engineering. Public
builds use only these models, public software dependencies and this repository.

Original hardware manuals, schematic drawings, datasheet files, OCR text, research renders, private
reference decks and document-extraction tooling remain in a local private folder excluded from Git and
release packaging. None is required to build this project. Do not add source documents to the public tree.

Release audits inspect the public Git history, candidate/indexed files and package payloads, including
restricted names supplied through a private term list. Required open-source dependency copyright and
licence notices are retained. The audit reports what was inspected; it does not replace a legal opinion
on intellectual-property rights.

Official binaries use the publisher's commercial framework licence and are freeware. The project's MIT
source terms and third-party component rights remain intact; see BINARY_LICENSE.txt and THIRD_PARTY_LICENSES.md.
