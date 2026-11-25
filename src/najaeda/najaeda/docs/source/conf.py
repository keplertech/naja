# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

import os
import re
import sys

from docutils import nodes
from sphinx.errors import ExtensionError

# Prefer the installed package (including its compiled extension). Fall back
# to the source package for local builds with the extension on PYTHONPATH.
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../../..')))


def read_naja_release():
    docs_source_dir = os.path.abspath(os.path.dirname(__file__))
    repo_root = os.path.abspath(os.path.join(docs_source_dir, '../../../../..'))
    version_file = os.path.join(repo_root, 'src/core/NajaVersion.h.in')
    with open(version_file, encoding='utf-8') as stream:
        content = stream.read()
    match = re.search(r'NAJA_VERSION\s*\{\s*"(?P<value>[^"]+)"\s*\}', content)
    if not match:
        raise RuntimeError(f'Cannot read NAJA_VERSION from {version_file}')
    return match.group('value')

project = 'najaeda'
copyright = '2024, Naja authors'
author = 'Naja authors'
release = read_naja_release()

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'sphinx.ext.autodoc',       # Enables the automodule directive
    'sphinx.ext.autosummary',   # Enables compact raw API indexes
    'sphinx.ext.napoleon',      # (Optional) Supports Google and NumPy-style docstrings
    'sphinx.ext.viewcode',      # (Optional) Links to source code in docs
    'sphinx.ext.todo',          # (Optional) For TODOs in the documentation
    'sphinx.ext.intersphinx',   # Resolve standard-library types
]

intersphinx_mapping = {'python': ('https://docs.python.org/3/', None)}
# Napoleon emits this parameter qualifier as a type; it is not an API name.
nitpick_ignore = [('py:class', 'optional')]
# Matrix fragments are application routes rather than HTML anchor IDs.
linkcheck_anchors_ignore_for_url = [r'https://matrix\.to/.*']

try:
    from najaeda import naja as raw_naja
except Exception as error:
    raise RuntimeError(
        "The documentation requires the compiled najaeda extension. "
        "Install this checkout with pip, or add the matching CMake Python "
        "extension directory to PYTHONPATH before running Sphinx."
    ) from error
sys.modules.setdefault("najaeda.naja", raw_naja)


def check_raw_api_links(app, doctree, docname):
    """Reject dangling raw API links, including targets removed by ``only``."""
    if docname != 'raw_api':
        return
    target_ids = {
        target_id
        for node in doctree.findall(nodes.Element)
        for target_id in node.get('ids', [])
    }
    missing = {
        node['refid']
        for node in doctree.findall(nodes.reference)
        if node.get('refid', '').startswith('najaeda.naja.')
        and node['refid'] not in target_ids
    }
    if missing:
        raise ExtensionError(
            'Raw API reference targets are missing: ' + ', '.join(sorted(missing))
        )


def check_raw_api_reference(app, env, node, contnode):
    target = node.get('reftarget', '')
    # The extension's internal type names differ from its public nested exports.
    aliases = {
        'SNLTermDirection': 'najaeda.naja.SNLTerm.Direction',
        'SNLNetType': 'najaeda.naja.SNLNet.Type',
        'SNLEquipotentialMode': 'najaeda.naja.SNLEquipotential.Mode',
        'SNLEquipotentialEdgeKind': 'najaeda.naja.SNLEquipotential.EdgeKind',
    }
    if target in aliases:
        return env.domains['py'].resolve_xref(
            env, node['refdoc'], app.builder, 'class', aliases[target], node, contnode
        )
    if target.startswith('najaeda.naja.'):
        raise ExtensionError(f'Unresolved raw API reference: {target}')


def resolve_source_backlinks(app, env):
    """Point source links back to indexed APIs rather than repeated guide entries."""
    modules = getattr(env, '_viewcode_modules', {})
    objects = env.domains['py'].objects
    for module, entry in list(modules.items()):
        if not entry:
            # Viewcode's module index otherwise links to unavailable native source.
            del modules[module]
            continue
        _, _, used, refname = entry
        for name in list(used):
            target = objects.get(f'{refname}.{name}')
            if target:
                used[name] = target.docname


def setup(app):
    app.connect('doctree-resolved', check_raw_api_links)
    app.connect('missing-reference', check_raw_api_reference)
    app.connect('env-updated', resolve_source_backlinks)

templates_path = ['_templates']
exclude_patterns = []

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'

# Run preprocessing step if building in a specific environment
import os
print("Running preprocessing script for Sphinx documentation...")
preprocessor_script = os.path.abspath('./preprocessor.py')
source_dir = os.path.abspath('../../../examples')
source_rst = os.path.abspath('./examples.rst.in')
dest_rst = os.path.abspath('./examples.rst')

try:
    import subprocess
    subprocess.call([
        sys.executable, preprocessor_script, 
        '--source_dir', source_dir, 
        '--source_rst', source_rst, 
        '--dest_rst', dest_rst
    ])
    print("Preprocessing completed successfully.")
except Exception as e:
    print(f"Error during preprocessing: {e}")
