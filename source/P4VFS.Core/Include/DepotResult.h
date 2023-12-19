// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "DepotName.h"
#pragma managed(push, off)

namespace Microsoft {
namespace P4VFS {
namespace P4 {
	
	struct IDepotClientCommand;

	enum class DepotResultChannel
	{
		None = 0,
		StdOut = 1<<0,
		StdErr = 1<<1,
	};

	enum class DepotResultReply
	{
		Unhandled = 0,
		Handled = 1,
	};

	struct FDepotResultTag
	{
		bool ContainsKey(const FDepotName& tagKey) const;
		void RemoveKey(const FDepotName& tagKey);
		void SetValue(const FDepotName& tagKey, const DepotString& tagValue);

		bool TryGetValue(const FDepotName& tagKey, DepotString& value) const;
		const DepotString& GetValue(const FDepotName& tagKey) const;
		const DepotString* GetValuePtr(const FDepotName& tagKey) const;

		int32_t GetValueInt32(const FDepotName& tagKey, int32_t defaultValue = 0) const;
		int64_t GetValueInt64(const FDepotName& tagKey, int64_t defaultValue = 0) const;

		typedef Map<FDepotName, DepotString> FieldsType;
		FieldsType m_Fields;
	};

	struct FDepotResultText
	{
		FDepotResultText(DepotResultChannel channel, const DepotString& value, int32_t level = 0);

		DepotResultChannel m_Channel;
		DepotString m_Value;
		int32_t m_Level;
	};

	typedef decltype(FDepotResultTag::m_Fields) DepotResultFields;
	typedef std::shared_ptr<FDepotResultTag> DepotResultTag;
	typedef std::shared_ptr<FDepotResultText> DepotResultText;
	typedef std::shared_ptr<struct FDepotResult> DepotResult;

	struct FDepotResult
	{
		P4VFS_CORE_API FDepotResult();
		virtual ~FDepotResult();

		virtual DepotResultReply OnComplete();
		virtual DepotResultReply OnStreamInfo(IDepotClientCommand* cmd, char level, const char* data);
		virtual DepotResultReply OnStreamOutput(IDepotClientCommand* cmd, const char* data, size_t length);
		virtual DepotResultReply OnStreamInput(IDepotClientCommand* cmd, DepotString& data);
		virtual DepotResultReply OnStreamStat(IDepotClientCommand* cmd, const DepotResultTag& tag);
		
		virtual bool HasError() const;
		virtual bool HasErrorRegex(const char* pattern) const;
		virtual void SetError(const char* errorText);
		virtual DepotString GetError() const;

		virtual bool HasText(DepotResultChannel channel) const;
		virtual DepotString GetText(DepotResultChannel channel) const;

		P4VFS_CORE_API const Array<DepotResultTag>& TagList() const;
		P4VFS_CORE_API const Array<DepotResultText>& TextList() const;
		const DepotString& GetTagValue(const DepotString& tagKey) const;
	
	protected:
		friend class DepotClientCommand;	
		Array<DepotResultTag> m_TagList;
		Array<DepotResultText> m_TextList;
	};

	struct FDepotResultNode
	{
		const FDepotResultTag& Tag() const
		{
			static const FDepotResultTag Empty;
			return m_Tag.get() ? *m_Tag : Empty;
		}

		bool ContainsTagKey(const FDepotName& tagKey) const
		{
			return Tag().ContainsKey(tagKey);
		}

		void RemoveTagKey(const FDepotName& tagKey)
		{
			if (m_Tag.get())
			{
				m_Tag->RemoveKey(tagKey);
			}
		}

		void SetTagValue(const FDepotName& tagKey, const DepotString& tagValue)
		{
			if (m_Tag.get() == nullptr)
			{
				m_Tag = std::make_shared<FDepotResultTag>();
			}
			m_Tag->SetValue(tagKey, tagValue);
		}

		const DepotString& GetTagValue(const FDepotName& tagKey) const
		{
			return Tag().GetValue(tagKey);
		}

		int32_t GetTagValueInt32(const FDepotName& tagKey, int32_t defaultValue = 0) const
		{
			return Tag().GetValueInt32(tagKey, defaultValue);
		}

		int64_t GetTagValueInt64(const FDepotName& tagKey, int64_t defaultValue = 0) const
		{
			return Tag().GetValueInt64(tagKey, defaultValue);
		}

		bool IsEmpty() const
		{
			return m_Tag.get() == nullptr || m_Tag->m_Fields.size() == 0;
		}

		template <typename T>
		static T Create(const DepotResultTag& tag)
		{
			T node;
			node.m_Tag = tag;
			return node;
		}
	
	private:
		DepotResultTag m_Tag;
	};

	template <typename NodeType>
	struct FDepotResultNodeProvider : FDepotResult
	{
		size_t NodeCount() const
		{
			return m_TagList.size();
		}

		NodeType Node(size_t index = 0) const
		{
			return FDepotResultNode::Create<NodeType>(index < m_TagList.size() ? m_TagList[index] : nullptr);
		}
	};
}}}

#pragma managed(pop)
