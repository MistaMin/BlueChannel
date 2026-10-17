#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 OpenGrid
"""Audit public source/history or an unpacked payload. Restricted terms are supplied privately."""
import argparse
import json
from pathlib import Path
import re
import subprocess

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--blocked-terms',type=Path)
p.add_argument('--payload',type=Path)
p.add_argument('--history',action='store_true')
p.add_argument('--output',type=Path)
a=p.parse_args()
root=Path(__file__).resolve().parents[1]
terms=[] if not a.blocked_terms else [x.strip().encode() for x in a.blocked_terms.read_text().splitlines() if x.strip() and not x.startswith('#')]
pattern=re.compile(b'|'.join(terms),re.I) if terms else None
secret=re.compile(rb'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----|gh[pousr]_[A-Za-z0-9]{30,}|AKIA[0-9A-Z]{16}')
issues=[];count=0;history_count=0

def inspect(name,data,originals=True):
    global count
    count+=1
    path=Path(name)
    if '.private' in path.parts or path.suffix.lower() in ('.p12','.pfx','.pem','.key','.mobileprovision'):
        issues.append({'file':name,'issue':'private material'})
    if originals and (path.suffix.lower() in ('.pdf','.cir','.sch','.asc') or data.startswith(b'%PDF-')):
        issues.append({'file':name,'issue':'original/reference document or deck'})
    if pattern and (pattern.search(name.encode()) or pattern.search(data)):
        issues.append({'file':name,'issue':'restricted name'})
    if secret.search(data):issues.append({'file':name,'issue':'credential-like material'})

if a.payload:
    for file in sorted(a.payload.rglob('*')):
        if file.is_file():inspect(file.relative_to(a.payload).as_posix(),file.read_bytes())
else:
    raw=subprocess.check_output(['git','ls-files','-z','--cached','--others','--exclude-standard'],cwd=root)
    for name in sorted(set(raw.decode().split('\0'))- {''}):
        file=root/name
        if file.is_file():inspect(name,file.read_bytes())
    # The linked toolkit is licensed source, not hardware reference material.
    toolkit=root/'third_party/GoodLookinUI'
    if (toolkit/'.git').exists():
        for name in subprocess.check_output(['git','ls-files','-z'],cwd=toolkit).decode().split('\0'):
            if name and (toolkit/name).is_file():inspect('third_party/GoodLookinUI/'+name,(toolkit/name).read_bytes())
if a.history:
    commit_messages=subprocess.check_output(['git','log','HEAD','--format=%B'],cwd=root)
    if pattern and pattern.search(commit_messages):issues.append({'file':'Git commit messages','issue':'restricted name'})
    for row in subprocess.check_output(['git','rev-list','--objects','HEAD'],cwd=root,text=True).splitlines():
        parts=row.split(' ',1)
        if len(parts)!=2:continue
        oid,name=parts
        if subprocess.check_output(['git','cat-file','-t',oid],cwd=root).strip()!=b'blob':continue
        inspect('history/'+name,subprocess.check_output(['git','cat-file','blob',oid],cwd=root));history_count+=1
result={'passed':not issues,'files_scanned':count,'history_blobs_scanned':history_count,'restricted_term_check':bool(terms),'issues':issues}
if a.output:a.output.write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
raise SystemExit(0 if not issues else 1)
