/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 University of Washington
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/ptr.h"
#include "ns3/node.h"
#include "ns3/net-device.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-address-generator.h"
#include "ns3/simulator.h"
#include "ipv4-address-helper-custom.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Ipv4AddressHelperCustom");

Ipv4AddressHelperCustom::Ipv4AddressHelperCustom (bool checkAddressDuplication) : m_checkAddressDuplication (checkAddressDuplication)
{
  NS_LOG_FUNCTION_NOARGS ();

//
// Set the default values to an illegal state.  Do this so the client is 
// forced to think at least briefly about what addresses get used and what
// is going on here.
//
  m_network = 0xffffffff;
  m_mask = 0;
  m_address = 0xffffffff;
  m_base = 0xffffffff;
  m_shift = 0xffffffff;
  m_max = 0xffffffff;
}

Ipv4AddressHelperCustom::Ipv4AddressHelperCustom (
  const Ipv4Address network, 
  const Ipv4Mask    mask,
  bool  checkAddressDuplication,
  const Ipv4Address address
) : m_checkAddressDuplication (checkAddressDuplication)
{
  NS_LOG_FUNCTION_NOARGS ();
  SetBase (network, mask, address);
}

void
Ipv4AddressHelperCustom::SetBase (
  const Ipv4Address network, 
  const Ipv4Mask mask,
  const Ipv4Address address)
{
  NS_LOG_FUNCTION_NOARGS ();

  m_network = network.Get ();
  m_mask = mask.Get ();
  m_base = m_address = address.Get ();

//
// Some quick reasonableness testing.
//
  NS_ASSERT_MSG ((m_network & ~m_mask) == 0,
                 "Ipv4AddressHelperCustom::SetBase(): Inconsistent network and mask");

//
// Figure out how much to shift network numbers to get them aligned, and what
// the maximum allowed address is with respect to the current mask.
//
  m_shift = NumAddressBits (m_mask);
  m_max = (1 << m_shift) - 2;

  NS_ASSERT_MSG (m_shift <= 32,
                 "Ipv4AddressHelperCustom::SetBase(): Unreasonable address length");

//
// Shift the network down into the normalized position.
//
  m_network >>= m_shift;

  NS_LOG_LOGIC ("m_network == " << m_network);
  NS_LOG_LOGIC ("m_mask == " << m_mask);
  NS_LOG_LOGIC ("m_address == " << m_address);
}

Ipv4Address
Ipv4AddressHelperCustom::NewAddress (void)
{
//
// The way this is expected to be used is that an address and network number
// are initialized, and then NewAddress() is called repeatedly to allocate and
// get new addresses on a given subnet.  The client will expect that the first
// address she gets back is the one she used to initialize the generator with.
// This implies that this operation is a post-increment.
//
  NS_ASSERT_MSG (m_address <= m_max,
                 "Ipv4AddressHelperCustom::NewAddress(): Address overflow");

  Ipv4Address addr ((m_network << m_shift) | m_address);
  ++m_address;
//
// The Ipv4AddressGenerator allows us to keep track of the addresses we have
// allocated and will assert if we accidentally generate a duplicate.  This
// avoids some really hard to debug problems.
//
  if (m_checkAddressDuplication)
    Ipv4AddressGenerator::AddAllocated (addr);
  return addr;
}

Ipv4Address
Ipv4AddressHelperCustom::NewNetwork (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  ++m_network;
  m_address = m_base;
  return Ipv4Address (m_network << m_shift);
}

Ipv4InterfaceContainer
Ipv4AddressHelperCustom::Assign (const NetDeviceContainer &c)
{
  NS_LOG_FUNCTION_NOARGS ();
  Ipv4InterfaceContainer retval;
  for (uint32_t i = 0; i < c.GetN (); ++i) {
      Ptr<NetDevice> device = c.Get (i);

      Ptr<Node> node = device->GetNode ();
      NS_ASSERT_MSG (node, "Ipv4AddressHelperCustom::Assign(): NetDevice is not not associated "
                     "with any node -> fail");

      Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
      NS_ASSERT_MSG (ipv4, "Ipv4AddressHelperCustom::Assign(): NetDevice is associated"
                     " with a node without IPv4 stack installed -> fail "
                     "(maybe need to use InternetStackHelper?)");

      int32_t interface = ipv4->GetInterfaceForDevice (device);
      if (interface == -1)
        {
          interface = ipv4->AddInterface (device);
        }
      NS_ASSERT_MSG (interface >= 0, "Ipv4AddressHelperCustom::Assign(): "
                     "Interface index not found");

      Ipv4InterfaceAddress ipv4Addr = Ipv4InterfaceAddress (NewAddress (), m_mask);
      ipv4->AddAddress (interface, ipv4Addr);
      ipv4->SetMetric (interface, 1);
      ipv4->SetUp (interface);
      retval.Add (ipv4, interface);
    }
  return retval;
}

const uint32_t N_BITS = 32; //!< number of bits in a IPv4 address

uint32_t
Ipv4AddressHelperCustom::NumAddressBits (uint32_t maskbits) const
{
  NS_LOG_FUNCTION_NOARGS ();
  for (uint32_t i = 0; i < N_BITS; ++i)
    {
      if (maskbits & 1)
        {
          NS_LOG_LOGIC ("NumAddressBits -> " << i);
          return i;
        }
      maskbits >>= 1;
    }

  NS_ASSERT_MSG (false, "Ipv4AddressHelperCustom::NumAddressBits(): Bad Mask");
  return 0;
}

} // namespace ns3

