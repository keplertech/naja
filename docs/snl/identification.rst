Objects identification
----------------------

Each Design DB object has a unique identifier:
`NLID <https://github.com/keplertech/naja/blob/main/src/nl/netlist/core/NLID.h>`_.

``NLID`` is a flat, model-level identifier. It identifies the database object
itself, not one particular occurrence of that object in an elaborated hierarchy.

+-------------+-----------+--------------+-------------------+
| Field       | Type      | Size (bytes) | Value range       |
+=============+===========+==============+===================+
| Object type | uint8_t   | 1            | 0-255             |
+-------------+-----------+--------------+-------------------+
| DB          | uint8_t   | 1            | 0-255             |
+-------------+-----------+--------------+-------------------+
| Library     | uint16_t  | 2            | 0 - 65535         |
+-------------+-----------+--------------+-------------------+
| Design      | uint32_t  | 4            | 0 - 4,294,967,295 |
+-------------+-----------+--------------+-------------------+
| Instance    | uint32_t  | 4            | 0 - 4,294,967,295 |
+-------------+-----------+--------------+-------------------+
| Net object  | uint32_t  | 4            | 0 - 4,294,967,295 |
+-------------+-----------+--------------+-------------------+
| Bit         | int32_t   | 4            | 0 - 4,294,967,295 |
+-------------+-----------+--------------+-------------------+

``Object type`` is ``NLID::Type``:
``DB=1, Library, Design, Term, TermBit, Net, NetBit, Instance, InstTerm``.
Two objects in the same design may share the same numeric id while having
different types, for example instance ``0`` and net ``0``. ``Type`` is the
discriminator that makes those identifiers distinct.

Each object ``NLID`` can be accessed with the ``getNLID()`` method.

**NLIDs** allow to:

- compare and sort objects.
- reference uniquely objects.
- access objects from SNLUniverse.

Model identity and hierarchical occurrence
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

There are two different identity questions:

* Which database object is this?
* Where does this object occur in the hierarchy?

``NLID`` answers the first question. A design can be instantiated many times, so
the ``NLID`` of an object in that design does not locate a specific occurrence
in the elaborated hierarchy.

To identify an object in the hierarchy, compose:

* the top design as an ``NLID::DesignReference`` ``(dbID, libraryID, designID)``;
* a vector of instance ids, one per level down the hierarchy;
* optionally, a leaf ``NLID::DesignObjectID`` for a terminal net, term, or
  instance term.

The instance-id vector uses ``SNLInstance::getID()`` values, which are unique
inside each parent design. This is the same sequence returned by
``SNLPath::getInstanceIDs()``.

``SNLOccurrence`` is the in-memory form of this composition: an ``SNLPath`` plus
the design object at that path. The top ``DesignReference`` and the instance-id
vector are stable across a naja-if dump and reload, so they can be persisted and
resolved in a later session.

.. note::

   Use an ``NLID`` for a single model-level object or flat handle. Use a top
   ``DesignReference`` plus an instance-id vector for an object in the elaborated
   hierarchy.

Resolving identities
~~~~~~~~~~~~~~~~~~~~

At the C++ level:

* ``NLUniverse::getObject(const NLID&)`` resolves any object from its
  model-level ``NLID``.
* ``NLUniverse::getSNLDesign(NLID::DesignReference)`` resolves a design from
  ``(dbID, libraryID, designID)``. This is the root of a hierarchical path.
* ``SNLDesign::getInstanceByID(id)`` resolves one level of a hierarchical path.
* ``SNLDesign::getTermByID(id)`` resolves a terminal in the current design.

At the Python level:

* ``NLUniverse.get().getObject(nid)`` resolves any object from an ``naja.NLID``.
* ``NLUniverse.get().getSNLDesign((dbID, libraryID, designID))`` resolves a
  design from a ``DesignReference`` tuple.
* ``design.getInstanceByID(id)`` resolves one level of a hierarchical path.
* ``design.getInstanceByIDList([id0, id1, ...])`` resolves an instance-id chain
  and returns the final instance.
* ``design.getTermByID(id)`` resolves a terminal in the current design.

For example:

.. code-block:: python

   # Hierarchical resolution from a top DesignReference and an instance-id vector.
   nid = top.getNLID()
   ref = (nid.getDBID(), nid.getLibraryID(), nid.getDesignID())

   design = naja.NLUniverse.get().getSNLDesign(ref)
   for instance_id in instance_ids:
       instance = design.getInstanceByID(instance_id)
       design = instance.getModel()

Python NLID
~~~~~~~~~~~

``object.getNLID()`` returns an ``naja.NLID`` value object.

``naja.NLID`` supports:

* comparisons, hashing, and printing: ``==``, ``<``, ``hash()``, ``str()``, and
  ``repr()``;
* use as a dictionary key or set member;
* the compact printed form ``NLID(type:db:lib:design:object:instance:bit)``.

The field accessors are:

* ``getType()``
* ``getDBID()``
* ``getLibraryID()``
* ``getDesignID()``
* ``getDesignObjectID()``
* ``getInstanceID()``
* ``getBit()``

The predicate helpers are:

* ``isDesign()``
* ``isInstance()``
* ``isNet()``
* ``isTerm()``

The ``NLID.Type`` enum values are also exported directly on ``NLID``:
``NLID.DB``, ``NLID.Library``, ``NLID.Design``, ``NLID.Term``,
``NLID.TermBit``, ``NLID.Net``, ``NLID.NetBit``, ``NLID.Instance``, and
``NLID.InstTerm``.

An ``NLID`` can be reconstructed from its seven fields or from its string
representation:

.. code-block:: python

   same_id = naja.NLID(*nid.toTuple())
   same_id = naja.NLID.fromString(str(nid))

Both forms reconstruct an equal id.
